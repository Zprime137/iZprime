/**
 * @brief Test function for T_STRUCT.
 *
 * This function tests the T_STRUCT implementation by performing a series of operations.
 * Tests initialization, appending, resizing, hashing, file I/O, and memory management.
 *
 * @param verbose If non-zero, prints detailed test output.
 * @return int 1 on success, 0 on failure.
 */
int T_TEST_FUNC(int verbose)
{
    char module_name[] = T_NAME;
    int passed_tests = 0;
    int failed_tests = 0;
    int current_test_idx = 0;

    print_test_module_header(module_name);

    if (verbose)
    {
        print_test_table_header();
    }

    // * Test 1: init
    current_test_idx++;

    int initial_capacity = 10;
    T_STRUCT *array = T_FUNC(init)(initial_capacity);
    if (array == NULL)
    {
        printf("%s failed critically at init. Aborting further tests.\n", T_NAME);
        return 0;
    }
    else
    {
        if (array->capacity == initial_capacity && array->count == 0 && array->array != NULL)
        {
            passed_tests++;
            if (verbose)
            {
                print_test_module_result(1, current_test_idx, "init", "Initialization with capacity %d successful", initial_capacity);
            }
        }
        else
        {
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "init", "Invalid initialization state");
            }
        }
    }

    // * Test 2: push (within capacity)
    current_test_idx++;
    for (int i = 0; i < initial_capacity; i++)
    {
        T_FUNC(push)(array, T_VAL(i));
    }

    if (array->count == initial_capacity)
    {
        int all_correct = 1;
        for (int i = 0; i < initial_capacity; i++)
        {
            if (array->array[i] != T_VAL(i))
            {
                all_correct = 0;
                if (verbose)
                {
                    print_test_module_result(0, current_test_idx, "push", "Element %d mismatch: expected " T_FMT ", got " T_FMT,
                             i, T_CAST(T_VAL(i)), T_CAST(array->array[i]));
                }
                failed_tests++;
                break;
            }
        }
        if (all_correct)
        {
            passed_tests++;
            if (verbose)
            {
                print_test_module_result(1, current_test_idx, "push", "All %d elements appended correctly", initial_capacity);
            }
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "push", "Count mismatch: expected %d, got %d",
                     initial_capacity, array->count);
        }
    }

    // * Test 3: sort
    current_test_idx++;

    T_STRUCT *sort_array = T_FUNC(init)(8);
    if (sort_array == NULL)
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "sort", "Failed to allocate sort test array");
        }
    }
    else
    {
        T_FUNC(push)(sort_array, T_VAL(5));
        T_FUNC(push)(sort_array, T_VAL(1));
        T_FUNC(push)(sort_array, T_VAL(4));
        T_FUNC(push)(sort_array, T_VAL(2));
        T_FUNC(push)(sort_array, T_VAL(3));
        T_FUNC(sort)(sort_array);

        int is_sorted = 1;
        for (int i = 1; i < sort_array->count; i++)
        {
            if (sort_array->array[i - 1] > sort_array->array[i])
            {
                is_sorted = 0;
                break;
            }
        }

        if (is_sorted)
        {
            passed_tests++;
            if (verbose)
            {
                print_test_module_result(1, current_test_idx, "sort", "Elements sorted in ascending order");
            }
        }
        else
        {
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "sort", "Sort produced non-monotonic output");
            }
        }

        T_FUNC(free)(&sort_array);
    }

    // * Test 4: resize
    current_test_idx++;

    int old_capacity = array->capacity;
    T_FUNC(push)(array, T_RESIZE_VAL); // This should trigger resize

    if (array->capacity > old_capacity && array->count == initial_capacity + 1)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "resize", "Automatic resize from %d to %d successful",
                     old_capacity, array->capacity);
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "resize", "Resize failed: capacity %d, count %d",
                     array->capacity, array->count);
        }
    }

    // * Test 5: Data integrity after resize
    current_test_idx++;

    int data_intact = 1;
    for (int i = 0; i < initial_capacity; i++)
    {
        if (array->array[i] != T_VAL(i))
        {
            data_intact = 0;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "resize_integrity", "Element %d corrupted after resize", i);
            }
            failed_tests++;
            break;
        }
    }
    if (data_intact && array->array[initial_capacity] == T_RESIZE_VAL)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "resize_integrity", "Data integrity preserved after resize");
        }
    }
    else if (data_intact)
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "resize_integrity", "New element not appended correctly");
        }
    }

    // * Test 6: pop
    current_test_idx++;
    int current_count = array->count;

    for (int i = 0; i < 3; i++)
    {
        T_FUNC(pop)(array);
    }
    if (array->count == current_count - 3)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "pop", "Pop operation successful, new count %d", array->count);
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "pop", "Pop operation failed, count %d", array->count);
        }
    }

    // * Test 7: compute_hash
    current_test_idx++;

    T_FUNC(compute_hash)(array);

    // Check if hash is non-zero
    int hash_computed = 0;
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        if (array->sha256[i] != 0)
        {
            hash_computed = 1;
            break;
        }
    }

    if (hash_computed)
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "compute_hash", "SHA-256 hash computed successfully");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "compute_hash", "Hash computation resulted in all zeros");
        }
    }

    // * Test 8: verify_hash
    current_test_idx++;

    if (T_FUNC(verify_hash)(array))
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "verify_hash", "Hash verification successful");
        }
    }
    else
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "verify_hash", "Hash verification failed");
        }
    }

    // * Test 9: fwrite
    current_test_idx++;

    const char *file_path = "./output/" T_NAME ".bin";
    FILE *file = fopen(file_path, "wb");
    if (file == NULL)
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "fwrite", "Failed to open file for writing");
        }
    }
    else
    {
        if (T_FUNC(fwrite)(array, file))
        {
            passed_tests++;
            if (verbose)
            {
                print_test_module_result(1, current_test_idx, "fwrite", "Array written to file successfully");
            }
        }
        else
        {
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "fwrite", "Failed to write array to file");
            }
        }
        fclose(file);
    }

    // Store count for verification after read
    int original_count = array->count;
    T_FUNC(free)(&array);

    // * Test 10: fread
    current_test_idx++;

    file = fopen(file_path, "rb");
    T_STRUCT *read_array = NULL;
    if (file == NULL)
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "fread", "Failed to open file for reading");
        }
    }
    else
    {
        read_array = T_FUNC(fread)(file);
        fclose(file);

        if (read_array == NULL)
        {
            failed_tests++;
            if (verbose)
            {
                print_test_module_result(0, current_test_idx, "fread", "Failed to read array from file");
            }
        }
        else
        {
            // Verify contents
            int content_valid = 1;
            if (read_array->count != original_count)
            {
                content_valid = 0;
                if (verbose)
                {
                    print_test_module_result(0, current_test_idx, "fread", "Count mismatch: expected %d, got %d",
                             original_count, read_array->count);
                }
            }
            else
            {
                for (int i = 0; i < read_array->count; i++)
                {
                    if (read_array->array[i] != T_VAL(i))
                    {
                        content_valid = 0;
                        if (verbose)
                        {
                            print_test_module_result(0, current_test_idx, "fread", "Element %d mismatch", i);
                        }
                        break;
                    }
                }
            }

            if (content_valid)
            {
                passed_tests++;
                if (verbose)
                {
                    print_test_module_result(1, current_test_idx, "fread", "Array read and contents verified");
                }
            }
            else
            {
                failed_tests++;
            }
        }
    }
    remove(file_path);

    // * Test 11: free
    current_test_idx++;

    T_FUNC(free)(&read_array);
    if (read_array != NULL || array != NULL)
    {
        failed_tests++;
        if (verbose)
        {
            print_test_module_result(0, current_test_idx, "free", "Pointer not nullified after free");
        }
    }
    else
    {
        passed_tests++;
        if (verbose)
        {
            print_test_module_result(1, current_test_idx, "free", "Memory freed and pointers nullified");
        }
    }

    // * Print test summary
    print_test_summary(module_name, passed_tests, failed_tests, verbose);

    return (failed_tests == 0) ? 1 : 0;
}
