#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

void ls_helper(struct dirent **pDirEnt, struct stat sb, const char *dir, int op_l, int op_R, int op_a, int op_A, int op_r, int indent, int i);

// getting number of blocks allocated
long long getSize(struct dirent **pDirEnt, const char *dir, int n)
{
    long long sum = 0;
    struct stat sb;

    for (int i = 0; i < n; i++)
    {
        if (strcmp(pDirEnt[i]->d_name, ".") == 0 || strcmp(pDirEnt[i]->d_name, "..") == 0)
            continue; // don't count . and ..
        char path[1024];

        snprintf(path, sizeof(path), "%s/%s", dir, pDirEnt[i]->d_name);
        stat(path, &sb);
        sum += (long long)sb.st_blocks;
    }
    return sum / 2; // divide by 2, because it is seen as 1024B blocks, but they are probably 512B blocks in reality
}

// function for getting all files listed
void _ls(const char *dir, int op_l, int op_R, int op_a, int op_A, int op_r, int indent)
{

    struct dirent **pDirEnt;
    struct stat sb;

    int n = scandir(dir, &pDirEnt, NULL, alphasort); // getting sorted files in pDirEnt and n as amount of files
    if (op_l)
        printf("total %lld\n", getSize(pDirEnt, dir, n));
    if (!op_r)
    {
        for (int i = 0; i < n; i++)
        { // if not -r, display alphabetically
            ls_helper(pDirEnt, sb, dir, op_l, op_R, op_a, op_A, op_r, indent, i);
        }
    }
    else
    { // if -r, display in reverse order
        for (int i = n - 1; i >= 0; i--)
        {
            ls_helper(pDirEnt, sb, dir, op_l, op_R, op_a, op_A, op_r, indent, i);
        }
    }
}

// function where all the magic happens
void ls_helper(struct dirent **pDirEnt, struct stat sb, const char *dir, int op_l, int op_R, int op_a, int op_A, int op_r, int indent, int i)
{
    if (op_A && (strcmp(pDirEnt[i]->d_name, ".") == 0 || strcmp(pDirEnt[i]->d_name, "..") == 0))
        return; // if -A, skip . and ..
    else if ((!op_a && !op_A) && pDirEnt[i]->d_name[0] == '.')
        return; // if not -a, skip hidden

    if (op_R && pDirEnt[i]->d_type == DT_DIR && strcmp(pDirEnt[i]->d_name, ".") != 0 && strcmp(pDirEnt[i]->d_name, "..") != 0)
    { // if directory and -R, recursion happens!
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, pDirEnt[i]->d_name);

        printf("%*s[%s]\n", indent, "", pDirEnt[i]->d_name);
        _ls(path, op_l, op_R, op_a, op_A, op_r, indent + 2); // indent is making subdirectories more visible
    }
    else
    {
        printf("%*s", indent, ""); // place indent on line
        if (op_l)
        { // -l stuff
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir, pDirEnt[i]->d_name);
            if (!stat(path, &sb))
            { // show permissions
                printf((S_ISDIR(sb.st_mode)) ? "d" : "-");
                printf((sb.st_mode & S_IRUSR) ? "r" : "-");
                printf((sb.st_mode & S_IWUSR) ? "w" : "-");
                printf((sb.st_mode & S_IXUSR) ? "x" : "-");
                printf((sb.st_mode & S_IRGRP) ? "r" : "-");
                printf((sb.st_mode & S_IWGRP) ? "w" : "-");
                printf((sb.st_mode & S_IXGRP) ? "x" : "-");
                printf((sb.st_mode & S_IROTH) ? "r" : "-");
                printf((sb.st_mode & S_IWOTH) ? "w" : "-");
                printf((sb.st_mode & S_IXOTH) ? "x" : "-");
            }
            else
            {
                perror("Error in stat");
            }
            printf(" %ld\t", (long)sb.st_nlink); // number of links

            long uid = (long)sb.st_uid; // get username by uid
            struct passwd *pwd;
            pwd = getpwuid(uid);

            printf("%s\t", pwd->pw_name);

            long gid = (long)sb.st_gid; // get group by gid
            struct group *grp;
            grp = getgrgid(gid);

            printf("%s\t", grp->gr_name);

            printf("%lld\t", (long long)sb.st_size); // show size of file

            char date[1024];
            strcpy(date, ctime(&sb.st_mtime)); // get date and cut it so only important information will be shown

            char newdate[1024] = {};
            for (int j = 0; j < 12; j++)
            {
                newdate[j] = date[j + 4];
            }
            printf("%s ", newdate);
        }
        else
        {
            printf("- ");
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, pDirEnt[i]->d_name);

        if (!stat(path, &sb))
        { // showing filename with color if its directory or is executable
            if (S_ISDIR(sb.st_mode))
            {
                printf("\033[1;34m%s\033[0m\n", pDirEnt[i]->d_name);
            }
            else if ((sb.st_mode & S_IXUSR) || (sb.st_mode & S_IXGRP) || (sb.st_mode & S_IXOTH))
            {
                printf("\033[1;32m%s\033[0m\n", pDirEnt[i]->d_name);
            }
            else
            {
                printf("%s\n", pDirEnt[i]->d_name);
            }
        }
    }
}

int main(int argc, char *argv[])
{

    char dir[1024] = ".";
    switch (argc)
    {
    case 1: // if no arguments were provided, show files in default settings
        _ls(dir, 0, 0, 0, 0, 0, 0);
        break;
    case 3: // check for path at second argument
        strcpy(dir, argv[2]);
    case 2:
        if (argv[1][0] == '-')
        { // get all settings provided by user after '-'
            int op_l = 0;
            int op_R = 0;
            int op_a = 0;
            int op_A = 0;
            int op_r = 0;

            char *option = (char *)(argv[1] + 1);

            while (*option)
            {
                switch (*option)
                {
                case 'l':
                    op_l = 1;
                    break;
                case 'R':
                    op_R = 1;
                    break;
                case 'a':
                    op_a = 1;
                    break;
                case 'A':
                    op_A = 1;
                    break;
                case 'r':
                    op_r = 1;
                    break;
                case 'h':
                    printf("NAME\n\tls - list directory contents\n\n");
                    printf("SYNOPSIS\n\tls [OPTION]... [FILE]...\n\n");
                    printf("DESCRIPTION\n\tList information about the FILEs (the current directory by default). Sort entries alphabetically.\n\n");
                    printf("\t-a\n\t\tdo not ignore entries starting with .\n");
                    printf("\t-A\n\t\tdo not list implied . and ..\n");
                    printf("\t-h\n\t\tdisplay this help and exit\n");
                    printf("\t-l\n\t\tuse a long listing format\n");
                    printf("\t-r\n\t\treverse order while sorting\n");
                    printf("\t-R\n\t\tlist subdirectories recursively\n");
                    return 0;
                default:
                    perror("Option not available");
                    exit(EXIT_FAILURE);
                    break;
                }
                option++;
            }
            _ls(dir, op_l, op_R, op_a, op_A, op_r, 0);
        }
        else
        { // if no '-' was found on first argument, instead get path from it
            _ls(argv[1], 0, 0, 0, 0, 0, 0);
        }
    default:
        break;
    }

    return 0;
}
