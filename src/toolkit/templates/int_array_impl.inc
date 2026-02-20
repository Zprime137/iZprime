/**
 * @file int_array_impl.inc
 * @brief Template implementation for generic integer arrays.
 */

#if !defined(TEMPLATE_TYPE) || !defined(TEMPLATE_STRUCT) || !defined(TEMPLATE_FUNC) || !defined(TEMPLATE_NAME_STR)
#error "Missing required macros for int_array_impl.inc template"
#endif

TEMPLATE_STRUCT *TEMPLATE_FUNC(init)(int capacity)
{
    assert(capacity > 0 && "Capacity must be positive value.");

    TEMPLATE_STRUCT *array = (TEMPLATE_STRUCT *)malloc(sizeof(TEMPLATE_STRUCT));
    if (array == NULL)
    {
        log_error("Memory allocation failed for %s structure.", TEMPLATE_NAME_STR);
        return NULL;
    }

    array->count = 0;
    array->capacity = capacity;
    array->ordered = 1; // New arrays are considered ordered by default

    array->array = (TEMPLATE_TYPE *)malloc(capacity * sizeof(TEMPLATE_TYPE));
    if (array->array == NULL)
    {
        log_error("Memory allocation failed for %s data array.", TEMPLATE_NAME_STR);
        free(array);
        return NULL;
    }

    memset(array->sha256, 0, SHA256_DIGEST_LENGTH);

    return array;
}

void TEMPLATE_FUNC(free)(TEMPLATE_STRUCT **array)
{
    if (array == NULL || *array == NULL)
        return;

    if ((*array)->array != NULL)
    {
        free((*array)->array);
        (*array)->array = NULL;
    }

    memset((*array)->sha256, 0, SHA256_DIGEST_LENGTH);

    free(*array);
    *array = NULL;
}

void TEMPLATE_FUNC(resize_to)(TEMPLATE_STRUCT *array, int new_capacity)
{
    assert(array && array->array && "Invalid array passed to resize_to.");
    
    if (new_capacity < array->count)
    {
        log_error("New capacity must be >= current count in %s resize_to.", TEMPLATE_NAME_STR);
        return;
    }

    TEMPLATE_TYPE *temp = realloc(array->array, new_capacity * sizeof(TEMPLATE_TYPE));
    if (temp == NULL)
    {
        log_error("Memory reallocation failed for %s data array.", TEMPLATE_NAME_STR);
        return;
    }
    array->array = temp;
    array->capacity = new_capacity;
}

void TEMPLATE_FUNC(resize_to_fit)(TEMPLATE_STRUCT *array)
{
    TEMPLATE_FUNC(resize_to)(array, array->count);
}

void TEMPLATE_FUNC(push)(TEMPLATE_STRUCT *array, TEMPLATE_TYPE element)
{
    assert(array && array->array && "Invalid array passed to push.");

    if (array->count == array->capacity)
    {
        int new_capacity = array->capacity + 1000;
        TEMPLATE_FUNC(resize_to)(array, new_capacity);
    }

    array->array[array->count++] = element;
}

static int TEMPLATE_FUNC(sort_cmp)(const void *lhs, const void *rhs)
{
    TEMPLATE_TYPE a = *(const TEMPLATE_TYPE *)lhs;
    TEMPLATE_TYPE b = *(const TEMPLATE_TYPE *)rhs;
    return (a > b) - (a < b);
}

void TEMPLATE_FUNC(sort)(TEMPLATE_STRUCT *array)
{
    assert(array && array->array && "Invalid array passed to sort.");

    if (array->count <= 1)
        return;

    qsort(array->array, (size_t)array->count, sizeof(TEMPLATE_TYPE), TEMPLATE_FUNC(sort_cmp));
}

void TEMPLATE_FUNC(pop)(TEMPLATE_STRUCT *array)
{
    assert(array && array->array && "Invalid array passed to pop.");

    if (array->count == 0)
    {
        return;
    }

    array->count--;
}

void TEMPLATE_FUNC(compute_hash)(TEMPLATE_STRUCT *array)
{
    assert(array && array->array && "Invalid array passed to compute_hash.");
    
    if (array->count == 0) {
        assert(array->count > 0 && "Array is empty in compute_hash.");
    }

    SHA256((unsigned char *)array->array, array->count * sizeof(TEMPLATE_TYPE), array->sha256);
}

int TEMPLATE_FUNC(verify_hash)(TEMPLATE_STRUCT *array)
{
    assert(array && array->array && "Invalid array passed to verify_hash.");
    assert(array->count > 0 && "Array is empty in verify_hash.");

    unsigned char computed_hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)array->array, array->count * sizeof(TEMPLATE_TYPE), computed_hash);
    if (memcmp(computed_hash, array->sha256, SHA256_DIGEST_LENGTH) != 0)
    {
        log_error("SHA-256 hash mismatch in %s.", TEMPLATE_NAME_STR);
        return 0;
    }
    return 1;
}

int TEMPLATE_FUNC(fwrite)(TEMPLATE_STRUCT *array, FILE *file)
{
    assert(array && array->array && "Invalid array passed to fwrite.");
    assert(file && "File pointer is NULL in fwrite.");

    TEMPLATE_FUNC(compute_hash)(array);

    if (fwrite(&array->count, sizeof(int), 1, file) != 1)
    {
        log_error("Failed to write count in %s fwrite.", TEMPLATE_NAME_STR);
        return 0;
    }

    if (fwrite(array->array, sizeof(TEMPLATE_TYPE), array->count, file) != (size_t)array->count)
    {
        log_error("Failed to write array data in %s fwrite.", TEMPLATE_NAME_STR);
        return 0;
    }

    if (fwrite(array->sha256, 1, SHA256_DIGEST_LENGTH, file) != SHA256_DIGEST_LENGTH)
    {
        log_error("Failed to write SHA-256 hash in %s fwrite.", TEMPLATE_NAME_STR);
        return 0;
    }

    return 1;
}

TEMPLATE_STRUCT *TEMPLATE_FUNC(fread)(FILE *file)
{
    assert(file && "File pointer is NULL in fread.");

    int count;
    if (fread(&count, sizeof(int), 1, file) != 1)
    {
        log_error("Failed to read count in %s fread.", TEMPLATE_NAME_STR);
        return NULL;
    }

    if (count <= 0 || count > 1000000000)
    {
        log_error("Invalid count value read from file in %s fread: %d", TEMPLATE_NAME_STR, count);
        return NULL;
    }

    TEMPLATE_STRUCT *array = TEMPLATE_FUNC(init)(count);
    if (array == NULL)
    {
        return NULL;
    }

    array->count = count;

    if (fread(array->array, sizeof(TEMPLATE_TYPE), count, file) != (size_t)count)
    {
        log_error("Failed to read array data in %s fread.", TEMPLATE_NAME_STR);
        TEMPLATE_FUNC(free)(&array);
        return NULL;
    }

    if (fread(array->sha256, 1, SHA256_DIGEST_LENGTH, file) != SHA256_DIGEST_LENGTH)
    {
        log_error("Failed to read SHA-256 hash in %s fread.", TEMPLATE_NAME_STR);
        TEMPLATE_FUNC(free)(&array);
        return NULL;
    }

    if (!TEMPLATE_FUNC(verify_hash)(array))
    {
        TEMPLATE_FUNC(free)(&array);
        log_error("SHA-256 hash verification failed in %s fread.", TEMPLATE_NAME_STR);
        return NULL;
    }

    return array;
}
