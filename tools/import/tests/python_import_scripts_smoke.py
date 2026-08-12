import importlib.util
import pathlib
import struct
import sys
import tempfile

import numpy


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: python_import_scripts_smoke.py <repository-root>")

    repository = pathlib.Path(sys.argv[1]).resolve()
    asset_root = repository / "bin/assets"
    if not asset_root.is_dir():
        asset_root = repository / "assets"
    raw_module = load_module(
        "drishti_raw_script",
        asset_root / "scripts/import/raw/raw.py",
    )
    numpy_module = load_module(
        "drishti_numpy_script",
        asset_root / "scripts/import/numpy_array/numpy_array.py",
    )

    with tempfile.TemporaryDirectory(prefix="Drishti script test ") as temporary:
        test_directory = pathlib.Path(temporary) / "unicode-\u4e2d\u6587"
        test_directory.mkdir()

        raw_values = (numpy.arange(24, dtype=numpy.uint16) + 100).reshape(2, 3, 4)
        raw_path = test_directory / "sample volume.raw"
        with raw_path.open("wb") as output:
            output.write(struct.pack("<Biii", 2, 2, 3, 4))
            output.write(raw_values.tobytes())

        raw_module.set_files([str(raw_path)])
        require(raw_module.SET_FILES_TRANSACTIONAL, "RAW transaction flag is missing")
        require(isinstance(raw_module.vol.data, numpy.memmap), "RAW is not mapped")
        require(raw_module.get_grid_size() == (4, 3, 2), "RAW grid is wrong")
        require(raw_module.get_rawvalue(1, 2, 3) == 123, "RAW value is wrong")
        require(
            raw_module.get_raw_min_max() == (100, 123),
            "RAW observed range is wrong",
        )
        raw_slice = numpy.zeros(12, dtype=numpy.uint16)
        raw_module.get_depth_slice(1, raw_slice)
        require(numpy.array_equal(raw_slice, raw_values[1].reshape(-1)), "RAW slice is wrong")
        raw_histogram = numpy.zeros(65536, dtype=numpy.uint32)
        raw_module.get_histogram(raw_histogram)
        require(int(raw_histogram.sum()) == 24, "RAW histogram is wrong")
        require(
            numpy.array_equal(
                numpy.flatnonzero(raw_histogram), numpy.arange(100, 124)
            )
            and numpy.all(raw_histogram[100:124] == 1),
            "RAW exact histogram bins are wrong",
        )

        truncated_path = test_directory / "truncated.raw"
        with truncated_path.open("wb") as output:
            output.write(struct.pack("<Biii", 2, 2, 3, 4))
        try:
            raw_module.set_files([str(truncated_path)])
        except ValueError:
            pass
        else:
            raise AssertionError("Truncated RAW input was accepted")
        require(isinstance(raw_module.vol.data, numpy.memmap), "RAW rollback lost mapping")
        require(raw_module.get_rawvalue(1, 2, 3) == 123, "RAW rollback lost old data")

        raw_module.close()
        require(raw_module.vol.data is None, "RAW mapping was not released")
        renamed_raw_path = test_directory / "renamed sample volume.raw"
        raw_path.rename(renamed_raw_path)
        renamed_raw_path.rename(raw_path)

        numpy_values = (numpy.arange(60, dtype=numpy.int16) - 30).reshape(3, 4, 5)
        numpy_path = test_directory / "sample array.npy"
        numpy.save(numpy_path, numpy_values)
        numpy_module.set_files([str(numpy_path)])
        require(numpy_module.SET_FILES_TRANSACTIONAL, "NPY transaction flag is missing")
        require(isinstance(numpy_module.vol.data, numpy.memmap), "NPY is not mapped")
        require(numpy_module.get_grid_size() == (5, 4, 3), "NPY grid is wrong")
        require(numpy_module.get_rawvalue(2, 3, 4) == 29, "NPY value is wrong")
        numpy_slice = numpy.zeros(20, dtype=numpy.int16)
        numpy_module.get_depth_slice(2, numpy_slice)
        require(
            numpy.array_equal(numpy_slice, numpy_values[2].reshape(-1)),
            "NPY slice is wrong",
        )
        numpy_histogram = numpy.zeros(65536, dtype=numpy.uint32)
        numpy_module.get_histogram(numpy_histogram)
        require(int(numpy_histogram.sum()) == 60, "NPY histogram is wrong")
        expected_signed_bins = numpy.arange(-30, 30, dtype=numpy.int64) + 32768
        require(
            numpy.array_equal(
                numpy.flatnonzero(numpy_histogram), expected_signed_bins
            )
            and numpy.all(numpy_histogram[expected_signed_bins] == 1),
            "NPY signed 16-bit exact histogram bins are wrong",
        )

        low_dynamic_values = numpy.array(
            [[[100, 101], [102, 103]]], dtype=numpy.uint8
        )
        low_dynamic_path = test_directory / "low dynamic array.npy"
        numpy.save(low_dynamic_path, low_dynamic_values)
        numpy_module.set_files([str(low_dynamic_path)])
        low_dynamic_histogram = numpy.zeros(65536, dtype=numpy.uint32)
        numpy_module.get_histogram(low_dynamic_histogram)
        require(
            numpy.array_equal(
                numpy.flatnonzero(low_dynamic_histogram),
                numpy.array([100, 101, 102, 103]),
            )
            and numpy.all(low_dynamic_histogram[100:104] == 1),
            "NPY unsigned 8-bit values were stretched across histogram bins",
        )

        signed_byte_values = numpy.array(
            [[[-128, -5], [0, 127]]], dtype=numpy.int8
        )
        signed_byte_path = test_directory / "signed byte array.npy"
        numpy.save(signed_byte_path, signed_byte_values)
        numpy_module.set_files([str(signed_byte_path)])
        signed_byte_histogram = numpy.zeros(65536, dtype=numpy.uint32)
        numpy_module.get_histogram(signed_byte_histogram)
        signed_byte_bins = numpy.array([0, 123, 128, 255])
        require(
            numpy.array_equal(
                numpy.flatnonzero(signed_byte_histogram), signed_byte_bins
            )
            and numpy.all(signed_byte_histogram[signed_byte_bins] == 1),
            "NPY signed 8-bit exact histogram bins are wrong",
        )

        big_endian_values = numpy.array(
            [[[1, 256, 4097], [32768, 50000, 65535]]], dtype=">u2"
        )
        big_endian_path = test_directory / "big endian array.npy"
        numpy.save(big_endian_path, big_endian_values)
        numpy_module.set_files([str(big_endian_path)])
        big_endian_slice = numpy.zeros(6, dtype=numpy.uint16)
        numpy_module.get_depth_slice(0, big_endian_slice)
        require(
            numpy.array_equal(
                big_endian_slice,
                big_endian_values.astype(numpy.uint16).reshape(-1),
            ),
            "Big-endian NPY slice was not converted to native byte order",
        )
        big_endian_histogram = numpy.zeros(65536, dtype=numpy.uint32)
        numpy_module.get_histogram(big_endian_histogram)
        big_endian_bins = big_endian_values.astype(numpy.uint16).reshape(-1)
        require(
            numpy.array_equal(
                numpy.flatnonzero(big_endian_histogram), big_endian_bins
            )
            and numpy.all(big_endian_histogram[big_endian_bins] == 1),
            "Big-endian NPY exact histogram bins are wrong",
        )

        fortran_values = numpy.asfortranarray(
            numpy.arange(60, dtype=numpy.float32).reshape(3, 4, 5)
        )
        fortran_path = test_directory / "fortran array.npy"
        numpy.save(fortran_path, fortran_values)
        numpy_module.set_files([str(fortran_path)])
        fortran_slice = numpy.zeros(20, dtype=numpy.float32)
        numpy_module.get_depth_slice(1, fortran_slice)
        require(
            numpy.array_equal(fortran_slice.reshape(4, 5), fortran_values[1]),
            "Fortran-order NPY slice is wrong",
        )

        constant_values = numpy.full((2, 3, 4), 7.0, dtype=numpy.float32)
        constant_path = test_directory / "constant array.npy"
        numpy.save(constant_path, constant_values)
        numpy_module.set_files([str(constant_path)])
        require(
            numpy_module.get_raw_min_max() == (7.0, 7.0),
            "Constant NPY range is wrong",
        )
        constant_histogram = numpy.zeros(65536, dtype=numpy.uint32)
        numpy_module.get_histogram(constant_histogram)
        require(
            int(constant_histogram.sum()) == constant_values.size,
            "Constant NPY histogram is wrong",
        )

        mixed_finite_values = numpy.array(
            [[[numpy.nan, -2.0], [numpy.inf, 5.0]]], dtype=numpy.float32
        )
        mixed_finite_path = test_directory / "mixed finite array.npy"
        numpy.save(mixed_finite_path, mixed_finite_values)
        numpy_module.set_files([str(mixed_finite_path)])
        require(
            numpy_module.get_raw_min_max() == (-2.0, 5.0),
            "Mixed finite NPY range is wrong",
        )
        mixed_slice = numpy.zeros(4, dtype=numpy.float32)
        numpy_module.get_depth_slice(0, mixed_slice)
        require(numpy.isnan(mixed_slice[0]), "NPY NaN was not preserved")
        require(numpy.isposinf(mixed_slice[2]), "NPY infinity was not preserved")

        archive_path = test_directory / "unsupported.npz"
        numpy.savez(archive_path, volume=numpy_values)
        try:
            numpy_module.set_files([str(archive_path)])
        except ValueError:
            pass
        else:
            raise AssertionError("NPZ input was accepted")
        require(
            isinstance(numpy_module.vol.data, numpy.memmap),
            "NPY rollback lost mapping",
        )
        require(
            numpy_module.get_raw_min_max() == (-2.0, 5.0),
            "NPY rollback lost old metadata",
        )
        rollback_slice = numpy.zeros(4, dtype=numpy.float32)
        numpy_module.get_depth_slice(0, rollback_slice)
        require(numpy.isnan(rollback_slice[0]), "NPY rollback lost old data")

        numpy_module.close()
        require(numpy_module.vol.data is None, "NPY mapping was not released")
        renamed_numpy_path = test_directory / "renamed mixed finite array.npy"
        mixed_finite_path.rename(renamed_numpy_path)
        renamed_numpy_path.rename(mixed_finite_path)

    print("Python Import script smoke passed")


if __name__ == "__main__":
    main()
