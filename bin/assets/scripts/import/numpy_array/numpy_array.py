import os

import numpy


CHUNK_ELEMENTS = 1024 * 1024


class Volume:
    def __init__(self):
        self.flnms = []
        self.description = "NumPy .npy volume (memory mapped)"
        self.voxelUnit = "micron"
        self.voxelSize = (1.0, 1.0, 1.0)
        self.voxelType = 0
        self.depth = 0
        self.width = 0
        self.height = 0
        self.dim = (0, 0, 0)
        self.headerBytes = 0
        self.data = None
        self.dataMin = 0
        self.dataMax = 0
        self.rawMin = 0
        self.rawMax = 0
        self.histogram = None

    def setFiles(self, flnms):
        self.close()
        if len(flnms) != 1:
            raise ValueError("Select exactly one NumPy .npy file")

        self.flnms = flnms
        flnm = self.flnms[0]
        if os.path.splitext(flnm)[1].lower() != ".npy":
            raise ValueError(
                "Compressed .npz archives are not supported; select a 3D .npy array"
            )

        self.data = numpy.load(flnm, mmap_mode="r", allow_pickle=False)
        if not isinstance(self.data, numpy.ndarray):
            raise ValueError("The selected file does not contain a NumPy array")
        if self.data.ndim != 3:
            raise ValueError(
                f"A 3D array is required; the selected array has {self.data.ndim} dimensions"
            )
        if any(int(value) <= 0 for value in self.data.shape):
            raise ValueError("The NumPy array contains an empty dimension")

        dtype_key = (self.data.dtype.kind, self.data.dtype.itemsize)
        voxel_types = {
            ("u", 1): 0,
            ("i", 1): 1,
            ("u", 2): 2,
            ("i", 2): 3,
            ("i", 4): 4,
            ("f", 4): 5,
        }
        if dtype_key not in voxel_types:
            raise ValueError(
                "Supported NumPy dtypes are uint8, int8, uint16, int16, int32, and float32"
            )

        self.voxelType = voxel_types[dtype_key]
        self.depth, self.width, self.height = (
            int(value) for value in self.data.shape
        )
        self.dim = (self.height, self.width, self.depth)

    def close(self):
        data = self.data
        self.data = None
        self.histogram = None
        if isinstance(data, numpy.memmap):
            mapping = getattr(data, "_mmap", None)
            if mapping is not None:
                mapping.close()

    def calculate_statistics(self):
        flat = self.data.ravel(order="K")
        data_min = None
        data_max = None

        for start in range(0, flat.size, CHUNK_ELEMENTS):
            chunk = flat[start : start + CHUNK_ELEMENTS]
            if self.voxelType == 5:
                chunk = chunk[numpy.isfinite(chunk)]
                if chunk.size == 0:
                    continue
            chunk_min = chunk.min().item()
            chunk_max = chunk.max().item()
            data_min = chunk_min if data_min is None else min(data_min, chunk_min)
            data_max = chunk_max if data_max is None else max(data_max, chunk_max)

        if data_min is None or data_max is None:
            raise ValueError("The NumPy volume contains no finite voxel values")

        self.dataMin = data_min
        self.dataMax = data_max
        self.rawMin = data_min
        self.rawMax = data_max

        bin_count = 256 if self.voxelType < 2 else 65536
        histogram = numpy.zeros(bin_count, dtype=numpy.uint64)
        exact_integer_histogram = self.voxelType < 4
        signed_offset = 0
        if self.voxelType == 1:
            signed_offset = 128
        elif self.voxelType == 3:
            signed_offset = 32768

        histogram_range = (data_min, data_max)
        for start in range(0, flat.size, CHUNK_ELEMENTS):
            chunk = flat[start : start + CHUNK_ELEMENTS]
            if self.voxelType == 5:
                chunk = chunk[numpy.isfinite(chunk)]
                if chunk.size == 0:
                    continue
            if exact_integer_histogram:
                indices = chunk
                if signed_offset:
                    indices = chunk.astype(numpy.int64) + signed_offset
                counts = numpy.bincount(indices, minlength=bin_count)
            else:
                counts, _ = numpy.histogram(
                    chunk, bins=bin_count, range=histogram_range
                )
            histogram += counts.astype(numpy.uint64, copy=False)

        self.histogram = histogram

    def get_depth_slice(self, depth):
        return self.data[depth, :]

    def get_rawvalue(self, depth, width, height):
        return self.data[depth, width, height].item()


vol = Volume()
SET_FILES_TRANSACTIONAL = True


def init():
    pass


def set_files(flnms):
    global vol
    candidate = Volume()
    try:
        candidate.setFiles(flnms)
        candidate.calculate_statistics()
    except Exception:
        candidate.close()
        raise

    previous = vol
    vol = candidate
    previous.close()


def get_description():
    return vol.description


def get_voxel_unit():
    return vol.voxelUnit


def get_voxel_size():
    return vol.voxelSize


def get_voxel_type():
    return vol.voxelType


def get_header_bytes():
    return vol.headerBytes


def get_grid_size():
    return vol.dim


def get_raw_min_max():
    return (vol.rawMin, vol.rawMax)


def get_histogram(hist: numpy.ndarray):
    maximum = numpy.iinfo(numpy.uint32).max
    hist[: len(vol.histogram)] = numpy.minimum(vol.histogram, maximum).astype(
        numpy.uint32, copy=False
    )


def get_depth_slice(slc, slice: numpy.ndarray):
    target = slice.reshape((vol.width, vol.height))
    # Equivalent dtypes can differ only in byte order.  NumPy performs the
    # byte swap while copying into Drishti's native-endian output buffer.
    numpy.copyto(target, vol.get_depth_slice(slc), casting="equiv")


def get_rawvalue(d, w, h):
    return vol.get_rawvalue(d, w, h)


def close():
    vol.close()
