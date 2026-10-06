#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DEBUG

#ifdef DEBUG
#define ON_DEBUG(...) __VA_ARGS__
#else
#define ON_DEBUG(...)
#endif

const unsigned int CANARY_BEGIN = 0xBEDABED0;
const unsigned int CANARY_END = 0xBEDABED1;
const unsigned int CANARY_STACK_BEGIN = 0xEDAEDA0;
const unsigned int CANARY_STACK_END = 0xEDAEDA1;

typedef double StackElem_t;

typedef struct
{
    unsigned int canary_begin;

    #ifdef ON_DEBUG
        const char* name;
        const char* file;
        int         line;
        const char* function;
    #endif

    StackElem_t* data;
    size_t size;
    size_t capacity;
    unsigned int canary_end;
} Stack_t;

enum Error_status
{
    OKAY = 0,
    ERROR_NULL_PTR = 1,
    ERROR_CANARY_BEGIN = 2,
    ERROR_CANARY_END = 3,
    ERROR_ZERO_SIZE = 4,
    ERROR_ZERO_CAPACITY = 5,
    ERROR_SIZE_GREATER_CAPACITY = 6,
    ERROR_CANARY_BEGIN_STACK = 7,
    ERROR_CANARY_END_STACK = 8,
    ERROR_CHANGING_SIZE = 9
};

Error_status StackInit       (Stack_t* stk, int capacity
                              ON_DEBUG (, const char* stk_name, const char* file_name, unsigned int line, const char* function));
Error_status StackPush       (Stack_t* stk, StackElem_t value
                              ON_DEBUG (, const char* file_name, unsigned int line, const char* function));
Error_status ExpandStackSize (Stack_t* stk ON_DEBUG(, unsigned int line));
Error_status StackPop        (Stack_t* stk, StackElem_t* value
                              ON_DEBUG (, const char* file_name, unsigned int line, const char* function));
Error_status NarrowStackSize (Stack_t* stk ON_DEBUG(, unsigned int line));
Error_status StackVerify     (Stack_t* stk ON_DEBUG(, unsigned int line));
void StackDestroy            (Stack_t* stk);
void StackDump               (Stack_t* stk, const char* error, unsigned int line, const char* function);

int main ()
{
    Stack_t stk1 = {};

    StackInit (&stk1, 10 ON_DEBUG (, "stk1", __FILE__, __LINE__, __PRETTY_FUNCTION__));

    StackPush (&stk1, 10 ON_DEBUG (, __FILE__, __LINE__, __PRETTY_FUNCTION__));
    StackPush (&stk1, 20 ON_DEBUG (, __FILE__, __LINE__, __PRETTY_FUNCTION__));
    StackPush (&stk1, 30 ON_DEBUG (, __FILE__, __LINE__, __PRETTY_FUNCTION__));
    //StackPush (&stk1, 40 ON_DEBUG (, __FILE__, __LINE__, __PRETTY_FUNCTION__));

    StackElem_t a;
    Error_status err1 = StackPop (&stk1, &a ON_DEBUG (, __FILE__, __LINE__, __PRETTY_FUNCTION__));
    StackElem_t b;
    //Error_status err2 = StackPop (&stk1, &b ON_DEBUG (, __FILE__, __LINE__, __PRETTY_FUNCTION__));
    //Error_status err3 = StackPop (&stk1, &b ON_DEBUG (, __FILE__, __LINE__, __PRETTY_FUNCTION__));
    //Error_status err4 = StackPop (&stk1, &b ON_DEBUG (, __FILE__, __LINE__, __PRETTY_FUNCTION__));
    //Error_status err3 = StackPop (&stk1, &b ON_DEBUG (, __FILE__, __LINE__, __PRETTY_FUNCTION__));

    StackDump (&stk1, "(null)", __LINE__, __FUNCTION__);
    return 0;
}

Error_status StackInit (Stack_t* stk, int capacity
                        ON_DEBUG (, const char* stk_name, const char* file, unsigned int line, const char* function))
{
    if (!stk)
        return ERROR_NULL_PTR;

    ON_DEBUG (
        stk->name = stk_name;
        stk->file = file;
        stk->line = line;
        stk->function = function;)

    stk->canary_begin = CANARY_BEGIN;
    stk->canary_end = CANARY_END;

    stk->data = (StackElem_t*)calloc(capacity + 2, sizeof (*stk->data));
    if (stk->data == NULL)
        return ERROR_NULL_PTR;
    stk->size = 0;
    stk->capacity = capacity;

    stk->data[0] = CANARY_STACK_BEGIN;
    stk->data[capacity + 1] = CANARY_STACK_END;
    for (size_t i = 1; i <= capacity; i++)
        stk->data[i] = NAN;

    Error_status error_code_1 = StackVerify (stk ON_DEBUG(, line));
    if (error_code_1 != OKAY)
        return error_code_1;

    return OKAY;
}

Error_status StackPush (Stack_t* stk, StackElem_t value
                        ON_DEBUG (, const char* file_name, unsigned int line, const char* function))
{
    Error_status error_code_1 = StackVerify (stk ON_DEBUG(, line));
    if (error_code_1 != OKAY)
        return error_code_1;

    if (stk->size >= stk->capacity) {
        ExpandStackSize (stk ON_DEBUG(, line));
    }

    stk->data[++stk->size] = value;

    Error_status error_code_2 = StackVerify (stk ON_DEBUG(, line));
    if (error_code_2 != OKAY)
        return error_code_2;

    //printf ("size: %lld, capacity: %lld\n", stk->size, stk->capacity);
    return OKAY;
}

Error_status ExpandStackSize (Stack_t* stk ON_DEBUG(, unsigned int line))
{
    int new_capacity = 2 * stk->capacity;
    StackElem_t* new_data = (StackElem_t*)realloc (stk->data, (new_capacity + 2) * sizeof (StackElem_t));
    if (new_data == NULL) {
        StackDump (stk, "Stack size increase error", __LINE__, __PRETTY_FUNCTION__);
        return ERROR_CHANGING_SIZE;
    } else {
        stk->data = new_data;
        stk->capacity = new_capacity;
    }
    for (size_t i = stk->size + 1; i <= new_capacity; i++)
        stk->data[i] = NAN;
    stk->data[new_capacity + 1] = CANARY_STACK_END;
    return OKAY;
}

Error_status StackPop (Stack_t* stk, StackElem_t* value
                       ON_DEBUG (, const char* file_name, unsigned int line, const char* function))
{
    Error_status error_code_1 = StackVerify (stk ON_DEBUG(, line));
    if (error_code_1 != OKAY)
        return error_code_1;

    if (stk->size == 0) {
        ON_DEBUG(StackDump(stk, "size = 0", line, __PRETTY_FUNCTION__);)
        return ERROR_ZERO_SIZE;
    }
    *value = stk->data[(stk->size)--];
    stk->data[stk->size + 1] = NAN;

    if (stk->size * 4 <= stk->capacity) {
        NarrowStackSize (stk ON_DEBUG(, line));
    }

    Error_status error_code_2 = StackVerify (stk ON_DEBUG(, line));
    if (error_code_2 != OKAY)
        return error_code_2;

    //printf ("size: %d, capacity: %d\n", stk->size, stk->capacity);
    return OKAY;
}

Error_status NarrowStackSize (Stack_t* stk ON_DEBUG(, unsigned int line))
{
    int new_capacity = stk->capacity / 2;
    StackElem_t* new_data = (StackElem_t*)realloc (stk->data, (new_capacity + 2) * sizeof (StackElem_t));
    if (new_data == NULL) {
        ON_DEBUG(StackDump (stk, "Stack size increase error", line, __PRETTY_FUNCTION__);)
        return ERROR_NULL_PTR;
    } else {
        stk->data = new_data;
        stk->capacity = new_capacity;
    }
    stk->data[stk->capacity + 1] = CANARY_STACK_END;

    return OKAY;
}

Error_status StackVerify (Stack_t* stk ON_DEBUG(, unsigned int line))
{
    if (stk == NULL) {
        ON_DEBUG(StackDump (stk, "Null pointer to the stack", line, __PRETTY_FUNCTION__);)
        return ERROR_NULL_PTR;
    }
    if (stk->canary_begin != CANARY_BEGIN) {
        ON_DEBUG(StackDump (stk, "Begin struct canary check fault", line, __PRETTY_FUNCTION__);)
        return ERROR_CANARY_BEGIN;
    }
    if (stk->canary_end != CANARY_END) {
        ON_DEBUG(StackDump (stk, "End struct canary check fault", line, __PRETTY_FUNCTION__);)
        return ERROR_CANARY_END;
    }
    if (stk->data == NULL) {
        ON_DEBUG(StackDump (stk, "Null pointer to the data", line, __PRETTY_FUNCTION__);)
        return ERROR_ZERO_SIZE;
    }
    if (stk->capacity == 0) {
        ON_DEBUG(StackDump (stk, "Capacity is less than 0", line, __PRETTY_FUNCTION__);)
        return ERROR_ZERO_CAPACITY;
    }
    if (stk->size > stk->capacity) {
        ON_DEBUG(StackDump (stk, "Size is greater than capacity", line, __PRETTY_FUNCTION__);)
        return ERROR_SIZE_GREATER_CAPACITY;
    }
    if (stk->data[0] != CANARY_STACK_BEGIN) {
        ON_DEBUG(StackDump (stk, "Canary at the beginning of the data is not okay", line, __PRETTY_FUNCTION__);)
        return ERROR_CANARY_BEGIN_STACK;
    }
    if (stk->data[stk->capacity + 1] != CANARY_STACK_END) {
        ON_DEBUG(StackDump (stk, "Canary at the end of the data is not okay", line, __PRETTY_FUNCTION__);)
        return ERROR_CANARY_END_STACK;
    }
    return OKAY;
}

void StackDestroy (Stack_t* stk)
{
    assert (stk);

    free (stk->data);
    stk->size = 0;
    stk->capacity = 0;
}

void StackDump (Stack_t* stk, const char* error, unsigned int line, const char* function)
{
    printf ("\033[36m---------------------- STACK DUMP -----------------------\033[0m\n"); //TODO цвета побольше
    printf ("Stack_t \"%s\" [%p] by \"%s\" at \"%s\":%u\n",
            stk->name, (void*)stk, stk->function, stk->file, stk->line);
    ON_DEBUG(printf ("\e[38;5;206mERROR: %s at function: \"%s\", line: %u\n\e[0m", error, function, line);)
    printf ("{\ncapacity = %lld\nsize = %lld\n", stk->capacity, stk->size);
    printf ("data [%p]\n", (void*)stk->data);
    printf ("{\n");
    for (size_t i = 1; i <= stk->capacity; i++)
    {
        if (i <= stk->size) {
            printf ("\033[33m   *[%lld] = %g\033[0m\n", i, stk->data[i]);
        }
        else {
            printf ("\033[32m    [%lld] = %g (POISON)\033[0m\n", i, stk->data[i]);
        }
    }
    printf ("}\n");
    printf ("\033[36m---------------------------------------------------------\033[0m\n");
}
