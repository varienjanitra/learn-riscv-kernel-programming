#include <user/user.h>

void main(void)
{
	while (1) {
prompt:
		printf("> ");

		char cmdline[128];
		for (int i = 0;; i++) {
			char ch = getchar();
			putchar(ch);

			if(i == sizeof(cmdline) - 1) {
				printf("Command line too long\n");
				goto prompt;
			} else if (ch == '\r') {
				printf("\n");
				cmdline[i] = '\0';
				break;
			} else {
				cmdline[i] = ch;
			}
		}

		if (strcmp(cmdline, "hello") == 0) {
			printf("Hello world from shell\n");
		} else if (strcmp(cmdline, "exit") == 0) {
			exit();
		} else if(strcmp(cmdline, "readfile") == 0) {
			char buf[128];
			int len = readfile("hello.txt", buf, sizeof(buf) - 1);

			if (len >= 0) {
				buf[len] = '\0';
				printf("%s\n", buf);
			} else {
				printf("Failed to read file\n");
			}
		} else if (strcmp(cmdline, "writefile") == 0) {
			const char *msg = "Hello from shell!\n";
			printf("%d", strlen(msg));
			writefile("hello.txt", (char *)msg, strlen(msg));
		}
		else
		 	printf("Unknown command: %s\n", cmdline);
	}
}
