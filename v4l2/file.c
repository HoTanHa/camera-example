
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
int main()
{
	char ch, file_name[50];
	FILE *fp;
	sprintf(file_name, "/home/hotanha/cam0_0231.f4v");

	printf("Enter name of a file you wish to see\n");
	//    gets(file_name);

	//    fp = fopen(file_name, "r"); // read mode

	//    if (fp == NULL)
	//    {
	//       perror("Error while opening the file.\n");
	//       exit(EXIT_FAILURE);
	//    }

	//    printf("The contents of %s file are:\n", file_name);

	// //    while((ch = fgetc(fp)) != EOF)
	//    char buff[10];
	//     while (fgets(buff, 1, fp)) {
	//       printf("%02X ", buff[0]);
	//    }

	//    fclose(fp);

	char c;
	int in, out;
	in = open(file_name, O_RDONLY);
	// out = open(“file.out”, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
	int x = 0;
	while (read(in, &c, 1) == 1)
	{
		printf("%02x ", c);
		if(x++>=1000)
			break;
		// write(out,&c,1)
	}

	printf("\r\n");
	return 0;
}