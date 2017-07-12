#ifndef _HOST_DRIVER_H_
#define _HOST_DRIVER_H_

int io_init();

int io_exit();

int io_open(char *file, int mode, SceMode mask);

int io_close(SceUID fd);

int usb_read_data(int fd, void *data, int len);

int io_read(SceUID fd, char *data, int len);

int usb_write_data(int fd, const void *data, int len);

int io_write(SceUID fd, const char *data, int len);

SceOff io_lseek(SceUID fd, SceOff ofs, int whence);

int io_ioctl(SceUID fd, unsigned int cmdno, void *indata, int inlen, void *outdata, int outlen);

int io_remove(const char *name);

int io_mkdir(const char *name, SceMode mode);

int io_rmdir(const char *name);

int io_dopen(const char *dir);

int io_dclose(SceUID fd);

int io_dread(SceUID fd, SceIoDirent *dir);

int io_getstat(const char *file, SceIoStat *stat);

int io_getstatbyfd(SceUID fd, SceIoStat *stat);

int io_chstat(const char *file, SceIoStat *stat, int bits);

int io_rename(const char *oldname, const char *newname);

int io_chdir(const char *dir);

int io_mount();

int io_umount();

int io_devctl(const char *name, unsigned int cmdno, void *indata, int inlen, void *outdata, int outlen);

#endif // _HOST_DRIVER_H_
