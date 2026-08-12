import os
import sys

import numpy


CHUNK_ELEMENTS = 1024 * 1024
HEADER_BYTES = 13
SUPPORTED_TYPES = {
    0: numpy.dtype("uint8"),
    2: numpy.dtype("<u2"),
}


class Volume:
    def __init__(self):
        self.flnms = []
        self.description = "RAW volume (memory mapped)"
        self.voxelUnit = "micron"
        self.voxelSize = (1.0, 1.0, 1.0)
        self.voxelType = 0
        self.bytesPerVoxel = 1
        self.depth = 0
        self.width = 0
        self.height = 0
        self.dim = (0, 0, 0)
        self.headerBytes = HEADER_BYTES
        self.data = None
        self.dataMin = 0
        self.dataMax = 0
        self.rawMin = 0
        self.rawMax = 0
        self.histogram = None

    def setFiles(self, flnms):
        self.close()
        if len(flnms) != 1:
            raise ValueError("Select exactly one Drishti RAW file")

        self.flnms = flnms
        flnm = self.flnms[0]
        with open(flnm, "rb") as fin:
            header = fin.read(HEADER_BYTES)

        if len(header) != HEADER_BYTES:
            raise ValueError("The RAW header is truncated")

        self.voxelType = header[0]
        if self.voxelType not in SUPPORTED_TYPES:
            raise ValueError(
                "The RAW script supports only unsigned 8-bit and 16-bit volumes"
            )

        dimensions = numpy.frombuffer(header, dtype="<i4", count=3, offset=1)
        self.depth, self.width, self.height = (int(value) for value in dimensions)
        if self.depth <= 0 or self.width <= 0 or self.height <= 0:
            raise ValueError("The RAW header contains invalid volume dimensions")

        voxel_count = self.depth * self.width * self.height
        dtype = SUPPORTED_TYPES[self.voxelType]
        expected_bytes = HEADER_BYTES + voxel_count * dtype.itemsize
        actual_bytes = os.path.getsize(flnm)
        if actual_bytes < expected_bytes:
            raise ValueError(
                "The RAW payload is truncated: expected at least "
                f"{expected_bytes} bytes, found {actual_bytes}"
            )

        self.bytesPerVoxel = dtype.itemsize
        self.dim = (self.height, self.width, self.depth)
        self.data = numpy.memmap(
            flnm,
            dtype=dtype,
            mode="r",
            offset=HEADER_BYTES,
            shape=(voxel_count,),
        )

        info = numpy.iinfo(dtype)
        self.rawMin = int(info.min)
        self.rawMax = int(info.max)

    def close(self):
        data = self.data
        self.data = None
        self.histogram = None
        if isinstance(data, numpy.memmap):
            mapping = getattr(data, "_mmap", None)
            if mapping is not None:
                mapping.close()

    def calculate_statistics(self):
        if self.data is None or self.data.size == 0:
            raise ValueError("The RAW volume contains no voxels")

        bin_count = 256 if self.voxelType == 0 else 65536
        histogram = numpy.zeros(bin_count, dtype=numpy.uint64)
        data_min = None
        data_max = None

        for start in range(0, self.data.size, CHUNK_ELEMENTS):
            chunk = self.data[start : start + CHUNK_ELEMENTS]
            chunk_min = int(chunk.min())
            chunk_max = int(chunk.max())
            data_min = chunk_min if data_min is None else min(data_min, chunk_min)
            data_max = chunk_max if data_max is None else max(data_max, chunk_max)
            counts = numpy.bincount(chunk, minlength=bin_count)
            histogram += counts[:bin_count].astype(numpy.uint64, copy=False)

        self.dataMin = data_min
        self.dataMax = data_max
        self.rawMin = data_min
        self.rawMax = data_max
        self.histogram = histogram

    def get_depth_slice(self, depth):
        slice_size = self.width * self.height
        start = depth * slice_size
        return self.data[start : start + slice_size]

    def get_rawvalue(self, depth, width, height):
        position = depth * self.width * self.height + width * self.height + height
        return self.data[position].item()


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
    numpy.copyto(slice, vol.get_depth_slice(slc), casting="no")


def get_rawvalue(d, w, h):
    return vol.get_rawvalue(d, w, h)


def close():
    vol.close()
