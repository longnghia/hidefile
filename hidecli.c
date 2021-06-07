#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc!=2){
        printf("Nhap duong dan (khong chua dau cach)\n");
        return 1;
    }

    FILE *fd = fopen("/dev/hidefile", "w");
    if (!fd)
    {
        printf("Failed to open the device...");
        return 1;
    }

    if (!fopen(argv[1],"r")){
        printf("File not found!\n");
        return 1;
    }

    int res = fprintf(fd, "%s", argv[1]); // Send the string to the char device
    if (res < 0)
    {
        printf("Failed to write the message to the device.");
        return 1;
    }

    printf("Your file has been hidden.\n");
    fclose(fd);

    return 0;
}
