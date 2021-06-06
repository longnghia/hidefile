#include <stdio.h>
// #include <stdlib.h>
#include <errno.h>
// #include <fcntl.h>
// #include <string.h>
// #include <unistd.h>

#define BUFFER_LENGTH 256
int main(int argc, char** argv)
{
    // char filePath[BUFFER_LENGTH];
    if (argc!=2){
        printf("Nhap duong dan (khong chua dau cach)\n");
        return 0;
    }

    FILE *fd = fopen("/dev/hidefile", "w");
    if (!fd)
    {
        perror("Failed to open the device...");
        return errno;
    }
    // printf("Enter path of the file you want to hide:\n");
    // scanf("%[^\n]%*c", filePath);          // Read in a string (with spaces)
    int res = fprintf(fd, "%s", argv[1]); // Send the string to the LKM
    if (res < 0)
    {
        perror("Failed to write the message to the device.");
        return errno;
    }

    printf("Your file has been hidden.\n");
    fclose(fd);

    return 0;
}

int main2()
{
    char filePath[BUFFER_LENGTH];

    FILE *fd = fopen("/dev/hidefile", "w");
    if (fd == NULL)
    {
        perror("Failed to open the device...");
        return errno;
    }
    printf("Enter path of the file you want to hide:\n");
    scanf("%[^\n]%*c", filePath);          // Read in a string (with spaces)
    int res = fprintf(fd, "%s", filePath); // Send the string to the LKM
    if (res < 0)
    {
        perror("Failed to write the message to the device.");
        return errno;
    }

    printf("Your file has been hidden.\n");
    fclose(fd);

    return 0;
}

