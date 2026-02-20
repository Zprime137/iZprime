#include <bitmap.h>

/**
 * @brief Test function for BITMAP.
 *
 * This function tests the BITMAP implementation by performing a series of operations.
 * Creates, clones, and frees a BITMAP, sets and clears bits, computes and validates the SHA-256 hash,
 * writes to a file, reads from the file, and verifies the contents.
 *
 * @param verbose If non-zero, prints detailed test output.
 * @return int 1 on success, 0 on failure.
 */
int TEST_BITMAP(int verbose)
{
    char module_name[] = "BITMAP";

    int passed_tests = 0;
    int failed_tests = 0;
    int current_test_idx = 0;
    int current_test_result = 1;

    print_test_module_header(module_name);

    if (verbose)
        print_test_table_header();

    // * Test 1: bitmap_init
    current_test_idx++;
    current_test_result = 1;

    size_t test_size = 1000;
    BITMAP *bitmap = bitmap_init(test_size, 0);
    if (bitmap == NULL)
    {
        printf("TEST_BITMAP failed critically at bitmap_init. Aborting further tests.\n");
        return 0; // Cannot continue without a valid bitmap
    }
    else
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_init", "Initialization with size %zu bits successful", test_size);
        }
    }

    // * Test 2: bitmap_set_bit
    current_test_idx++;
    current_test_result = 1;

    for (size_t i = 0; i < test_size; i += 2)
    {
        bitmap_set_bit(bitmap, i);
        if (bitmap_get_bit(bitmap, i) != 1)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "bitmap_set_bit", "Bit %zu not set correctly", i);
            }
            break;
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_set_bit", "All even-indexed bits set correctly");
        }
    }

    // * Test 3: bitmap_clone
    BITMAP *cloned_bitmap = bitmap_clone(bitmap);
    current_test_idx++;
    current_test_result = 1;
    if (cloned_bitmap == NULL)
    {
        current_test_result = 0;
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "bitmap_clone", "Cloning failed");
        }
    }
    else
    {
        // Verify cloned bitmap matches original
        for (size_t i = 0; i < test_size; i++)
        {
            if (bitmap_get_bit(bitmap, i) != bitmap_get_bit(cloned_bitmap, i))
            {
                current_test_result = 0;
                failed_tests++;
                if (verbose)
                {
                    print_test_module_result(0, current_test_idx, "bitmap_clone", "Bit %zu mismatch between original and clone", i);
                }
                break;
            }
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_clone", "Cloning successful and verified");
        }
    }
    bitmap_free(&cloned_bitmap);

    // * Test 4: bitmap_get_bit
    current_test_idx++;
    current_test_result = 1;
    for (size_t i = 0; i < test_size; i++)
    {
        int bit = bitmap_get_bit(bitmap, i);
        if (i % 2 == 0 && bit != 1)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "bitmap_get_bit", "Bit %zu should be set", i);
            }
            break;
        }
        else if (i % 2 != 0 && bit != 0)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "bitmap_get_bit", "Bit %zu should be clear", i);
            }
            break;
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_get_bit", "All bits read correctly");
        }
    }

    // * Test 5: bitmap_set_all
    current_test_idx++;
    current_test_result = 1;
    bitmap_set_all(bitmap);
    for (size_t i = 0; i < test_size; i++)
    {
        int bit = bitmap_get_bit(bitmap, i);
        if (bit != 1)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "bitmap_set_all", "Bit %zu not set", i);
            }
            break;
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_set_all", "All bits set to 1");
        }
    }

    // * Test 6: bitmap_clear_bit
    current_test_idx++;
    current_test_result = 1;
    for (size_t i = 0; i < test_size; i += 2)
    {
        bitmap_clear_bit(bitmap, i);
        if (bitmap_get_bit(bitmap, i) != 0)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "bitmap_clear_bit", "Bit %zu not cleared", i);
            }
            break;
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_clear_bit", "All even-indexed bits cleared");
        }
    }

    // * Test 7: bitmap_clear_all
    current_test_idx++;
    current_test_result = 1;
    bitmap_clear_all(bitmap);
    for (size_t i = 0; i < test_size; i++)
    {
        int bit = bitmap_get_bit(bitmap, i);
        if (bit != 0)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "bitmap_clear_all", "Bit %zu not cleared", i);
            }
            break;
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_clear_all", "All bits cleared to 0");
        }
    }

    // * Test 8: bitmap_flip_bit
    current_test_idx++;
    current_test_result = 1;
    for (size_t i = 0; i < test_size; i += 2)
    {
        bitmap_flip_bit(bitmap, i);
        if (bitmap_get_bit(bitmap, i) != 1)
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "bitmap_flip_bit", "Bit %zu not flipped correctly", i);
            }
            break;
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_flip_bit", "All even-indexed bits flipped correctly");
        }
    }

    // * Test 9: bitmap_clear_steps
    current_test_idx++;
    current_test_result = 1;
    bitmap_set_all(bitmap);                          // Set all bits first
    bitmap_clear_steps(bitmap, 3, 0, test_size - 1); // Clear every 3rd bit
    for (size_t i = 0; i < test_size; i++)
    {
        if (i % 3 == 0)
        {
            if (bitmap_get_bit(bitmap, i) != 0)
            {
                current_test_result = 0;
                failed_tests++;
                if (verbose)
                {
                    print_test_module_result(0, current_test_idx, "bitmap_clear_steps", "Bit %zu not cleared", i);
                }
                break;
            }
        }
        else
        {
            if (bitmap_get_bit(bitmap, i) != 1)
            {
                current_test_result = 0;
                failed_tests++;
                if (verbose)
                {
                    print_test_module_result(0, current_test_idx, "bitmap_clear_steps", "Bit %zu should be set", i);
                }
                break;
            }
        }
    }
    if (current_test_result)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_clear_steps", "Bits cleared in steps correctly");
        }
    }

    // * Test 10: bitmap_compute_hash and bitmap_validate_hash
    current_test_idx++;
    current_test_result = 1;
    bitmap_compute_hash(bitmap);
    if (!bitmap_validate_hash(bitmap))
    {
        current_test_result = 0;
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "bitmap_compute_hash", "SHA-256 computation failed");
        }
    }
    else
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_compute_hash", "SHA-256 computation successful and validated");
        }
    }

    // * Test 11: bitmap_fwrite
    current_test_idx++;
    current_test_result = 1;
    const char *file_path = "./output/TEST_BITMAP.bin";
    FILE *file = fopen(file_path, "wb");
    if (file == NULL)
    {
        current_test_result = 0;
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "bitmap_fwrite", "Failed to open file for writing");
        }
    }
    else
    {
        if (!bitmap_fwrite(bitmap, file))
        {
            current_test_result = 0;
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "bitmap_fwrite", "Failed to write bitmap to file");
            }
        }
        else
        {
            passed_tests++;
            if (verbose)
            {
                print_test_module_result(1, current_test_idx, "bitmap_fwrite", "Bitmap written to file successfully");
            }
        }
        fclose(file);
    }
    bitmap_free(&bitmap);

    // * Test 12: bitmap_fread
    current_test_idx++;
    current_test_result = 1;
    file = fopen(file_path, "rb");
    BITMAP *read_bitmap = NULL;
    if (file == NULL)
    {
        current_test_result = 0;
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "bitmap_fread", "Failed to open file for reading");
        }
    }
    else
    {
        read_bitmap = bitmap_fread(file);
        fclose(file);

        if (read_bitmap == NULL)
        {
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "bitmap_fread", "Failed to read bitmap from file");
            }
            current_test_result = 0;
            failed_tests++;
        }
        else
        {
            // Verify contents match what was written
            int content_valid = 1;
            for (size_t i = 0; i < test_size; i++)
            {
                int expected_bit = (i % 3 == 0) ? 0 : 1;
                if (bitmap_get_bit(read_bitmap, i) != expected_bit)
                {
                    content_valid = 0;
                    current_test_result = 0;
                    failed_tests++;
                    if (verbose)
                    {
                        print_test_module_result(0, current_test_idx, "bitmap_fread", "Bit %zu mismatch (expected %d)", i, expected_bit);
                    }
                    break;
                }
            }
            if (content_valid)
            {
                passed_tests++;
                if (verbose)
                {
                    print_test_module_result(1, current_test_idx, "bitmap_fread", "Bitmap read and contents verified");
                }
            }
        }
    }
    remove(file_path); // Clean up test file

    // * Test 13: bitmap_free
    current_test_idx++;
    current_test_result = 1;
    bitmap_free(&read_bitmap);
    if (read_bitmap != NULL || bitmap != NULL || cloned_bitmap != NULL)
    {
        current_test_result = 0;
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "bitmap_free", "Pointer not nullified after free");
        }
    }
    else
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "bitmap_free", "Memory freed and pointers nullified");
        }
    }

    // * Print test summary
    print_test_summary(module_name, passed_tests, failed_tests, verbose);

    return (failed_tests == 0) ? 1 : 0;
}
