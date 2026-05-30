/******************************************************************************
 * Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
 *
 * Redistribution, modification or use of this software in source or binary
 * forms is permitted as long as the files maintain this copyright. Users are
 * permitted to modify this and use it to learn about the field of embedded
 * software. Carlos Villarreal and CETYS Universidad are not liable for any
 * misuse of this material.
 *
 *****************************************************************************/
/**
 * @file utils.c
 * @brief Utility library with helper functions.
 *
 * Utils module has helper functions to treat strings, ASCII conversions, and
 * printing utilities.
 *
 * @author Oskar Liborio Garcia Veliz
 * @date 04/30/2026
 *
 */

/*** Includes ***/
#include "utils.h"

/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/*** Local Variables ***/

/*** External Variables ***/

/*** Function Prototypes ***/

static uint32_t utils_printString(char *dst, char *src);
static uint32_t utils_printInt(char *dst, int32_t num, uint8_t sign, uint32_t base);

/*** Function Definitions ***/

void utils_snprintf(char *dst, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    while (*format)
    {
        if (*format == '%')
        {
            format++;
            switch (*format)
            {
                /* Hint: on the data type cases use va_arg(args, data_type) */
                case 's':
                    /* Write your code here */
                    dst += utils_printString(dst, va_arg(args, char *));
                    break;
                case 'd':
                    /* Write your code here */
                    dst += utils_printInt(dst, (int32_t)va_arg(args, int32_t), 1U, 10U);
                    break;
                case 'u':
                    /* Write your code here */
                    dst += utils_printInt(dst, (int32_t)va_arg(args, uint32_t), 0U, 10U);
                    break;
                case 'x':
                    /* Write your code here */
                    dst += utils_printInt(dst, (int32_t)va_arg(args, uint32_t), 0U, 16U);
                    break;
                case 'c':
                    /* Write your code here */
                    *dst++ = (char)va_arg(args, int);
                    break;
                case '%':
                    /* Write your code here */
                    *dst++ = '%';
                    break;
                default:
                    /* Write your code here */
                    *dst++ = '%';
                    *dst++ = *format;
                    break;
            }
        }
        else
        {
            *dst++ = *format;
        }

        format++;
    }

    va_end(args);
}


/**
 * @brief Copy a string into a destination buffer.
 *
 * This function calculates the length of the source string and copies
 * its contents into the destination buffer.
 *
 * @param dst Pointer to the destination buffer where the string will be copied.
 * @param src Pointer to the source string to copy.
 *
 * @return Length of the string copied (number of characters).
 */
static uint32_t utils_printString(char *dst, char *src)
{
    /* Write your code here */
    uint32_t len = 0U;

    while (*src != 0)
    {
        (void)utils_memCpy(dst++, src++, 1U);
        len++;
    }

    return len;
}

/**
 * @brief Convert an integer to ASCII and copy it into a destination buffer.
 *
 * This function converts an integer into its ASCII representation
 * based on the specified base (2-16) and sign option.
 * The resulting string is copied into the destination buffer.
 *
 * @param dst Pointer to the destination buffer where the ASCII string will be stored.
 * @param num Integer number to convert.
 * @param sign Interger value that indicates if data is signed or unsigned.
 * @param base Numerical base for conversion.
 *
 * @return Length of the ASCII string copied into the destination buffer.
 */
static uint32_t utils_printInt(char *dst, int32_t num, uint8_t sign, uint32_t base)
{
    /* Write your code here */
    uint8_t  buf[32U];
    uint32_t len = 0U;
    uint32_t i;

    len = utils_itoa(num, buf, sign, (uint8_t)base);

    for (i = 0U; i < len; i++)
    {
        dst[i] = (char)buf[i];
    }

    return len;
}

uint32_t utils_itoa(int32_t data, uint8_t *ptr, uint8_t sign, uint8_t base)
{
    uint32_t len        = 0U;
    uint8_t  isNegative = 0U;
    uint32_t udata;
    uint8_t  digit;

    if ((sign != 0U) && (data < 0))
    {
        isNegative = 1U;
        udata = (uint32_t)(-data);
    }
    else
    {
        udata = (uint32_t)data;
    }

    if (udata == 0U)
    {
        ptr[len++] = (uint8_t)'0';
    }

    while (udata > 0U)
    {
        digit = (uint8_t)(udata % (uint32_t)base);
        ptr[len++] = (digit < 10U) ? ((uint8_t)((uint8_t)'0' + digit)) : ((uint8_t)((uint8_t)'A' + digit - 10U));
        udata /= (uint32_t)base;
    }

    if (isNegative == 1U)
    {
        ptr[len++] = (uint8_t)'-';
    }
    (void)utils_memReverse(ptr, (size_t)len);

    return len;
}

int32_t utils_atoi(uint8_t *ptr, uint32_t digits, uint8_t sign, uint8_t base)
{
    int32_t  result     = 0;
    uint8_t  isNegative = 0U;
    uint32_t i          = 0U;
    uint8_t  digit;
    uint8_t  c;

    if ((sign != 0U) && (ptr[0U] == (uint8_t)'-'))
    {
        isNegative = 1U;
        i = 1U;
    }

    for (; i < digits; i++)
    {
        c = ptr[i];

        if ((c >= (uint8_t)'0') && (c <= (uint8_t)'9'))
        {
            digit = c - (uint8_t)'0';
        }
        else if ((c >= (uint8_t)'a') && (c <= (uint8_t)'f'))
        {
            digit = c - (uint8_t)'a' + 10U;
        }
        else if ((c >= (uint8_t)'A') && (c <= (uint8_t)'F'))
        {
            digit = c - (uint8_t)'A' + 10U;
        }
        else
        {
            break;
        }

        result = (result * (int32_t)base) + (int32_t)digit;
    }

    return (isNegative == 1U) ? (-result) : result;
}

void * utils_memCpy(void *dst, void *src, size_t length)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    size_t         i;

    for (i = 0U; i < length; i++)
    {
        d[i] = s[i];
    }

    return dst;
}

void * utils_memReverse(void *src, size_t length)
{
    uint8_t *data  = (uint8_t *)src;
    size_t   left  = 0U;
    size_t   right = length - 1U;
    uint8_t  tmp;

    while (left < right)
    {
        tmp           = data[left];
        data[left]    = data[right];
        data[right]   = tmp;
        left++;
        right--;
    }

    return src;
}