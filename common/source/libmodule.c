//
// Created by cpasjuste on 07/11/16.
//

#ifdef MODULE
#include "libmodule.h"

#ifdef __VITA_KERNEL__
#include <psp2kern/kernel/sysmem.h>
static SceUID g_pool = -1;
#else
#include <psp2/kernel/sysmem.h>
#endif

#ifdef DEBUG
void mdebug(const char *fmt, ...) {

    SceUID s_debug_fd = sceIoOpen("ux0:tai/psp2shell.log", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (s_debug_fd >= 0) {
        char s_debug_msg[256];
        memset(s_debug_msg, 0, 256);
        va_list args;
        va_start(args, fmt);
        vsnprintf(s_debug_msg, 256, fmt, args);
        va_end(args);
        sceIoWrite(s_debug_fd, s_debug_msg, strlen(s_debug_msg));
        sceIoClose(s_debug_fd);
    }
}
#endif

void *malloc(size_t size) {
#ifdef __VITA_KERNEL__
    if(g_pool < 0) {
        SceKernelMemPoolCreateOpt opt;
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.uselock = 1;
        g_pool = sceKernelMemPoolCreate("m", 1024*10, &opt);
        if (g_pool < 0) {
            return NULL;
        }
    }
    void *ptr = sceKernelMemPoolAlloc(g_pool, size + sizeof(size_t));
    if (ptr) {
        *(size_t *)ptr = size;
        ptr = (char *)ptr + sizeof(size_t);
    }
    return ptr;
#else
    void *p = NULL;
    size = (size + 0xFFF) & (~0xFFF); // align 4K
    SceUID uid = sceKernelAllocMemBlock("m", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, size, 0);
    if (uid >= 0) {
        sceKernelGetMemBlockBase(uid, &p);
    }
    return p;
#endif
}

void *_realloc(void *ptr, size_t old_size, size_t new_size) {

    if (ptr == NULL) {
        return malloc(new_size);
    }
    void *newptr = malloc(new_size);
    if (newptr == NULL) {
        return NULL;
    }
    newptr = memcpy(newptr, ptr, old_size);
    free(ptr);

    return newptr;
}

void free(void *p) {
#ifdef __VITA_KERNEL__
    sceKernelMemPoolFree(g_pool, (char *)p - sizeof(size_t));
#else
    SceUID uid = sceKernelFindMemBlockByAddr(p, 0);
    if (uid >= 0) {
        sceKernelFreeMemBlock(uid);
    }
#endif
}

double atof(const char *str) {

    double sum = 0;

    while (*str) {
        if (*str != '.') {
            sum = (sum * 10) + (*str - '0');
            str++;
        }

        if (*str == '.') {
            double scale = 0.1;
            str++;
            while (*str) {
                sum += scale * (*str - '0');
                str++;
                scale *= 0.1;
            }
        }
    }

    return sum;
}

char *strdup(const char *s) {
    char *str;
    char *p;
    int len = 0;

    while (s[len])
        len++;
    str = malloc(len + 1);
    p = str;
    while (*s)
        *p++ = *s++;
    *p = '\0';
    return str;
}

//
typedef unsigned char u_char;
static const u_char charmap[] = {
        '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
        '\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
        '\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
        '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
        '\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
        '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
        '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
        '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
        '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
        '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
        '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
        '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
        '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
        '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
        '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
        '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
        '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
        '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
        '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
        '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
        '\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
        '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
        '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
        '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
        '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
        '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
        '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
        '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
        '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
        '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
        '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
        '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

int strcasecmp(s1, s2)
        const char *s1, *s2;
{
    register const u_char *cm = charmap,
            *us1 = (const u_char *) s1,
            *us2 = (const u_char *) s2;

    while (cm[*us1] == cm[*us2++])
        if (*us1++ == '\0')
            return (0);
    return (cm[*us1] - cm[*--us2]);
}

int strncasecmp(s1, s2, n)
        const char *s1, *s2;
        register size_t n;
{
    if (n != 0) {
        register const u_char *cm = charmap,
                *us1 = (const u_char *) s1,
                *us2 = (const u_char *) s2;

        do {
            if (cm[*us1] != cm[*us2++])
                return (cm[*us1] - cm[*--us2]);
            if (*us1++ == '\0')
                break;
        } while (--n != 0);
    }
    return (0);
}

#endif // MODULE