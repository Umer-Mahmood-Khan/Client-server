

#include <stdio.h>    
#include <sys/socket.h>           // Socket functions
#include <arpa/inet.h>            // Internet address functions
#include <unistd.h>               
#include <sys/wait.h>             
#include <errno.h>                
#include <sys/stat.h>    
#include <stdlib.h>               
#include <string.h>               // String manipulation functions
#include <time.h>                 
#include <dirent.h>    
#include <sys/types.h>           

#define PORT 8080                 // Port number for main server
#define BUFSIZE 1024              // The size of buffer
#define MIRROR_PORT 7001          // Port number for mirror server

// Function to search for a file in directories
void npsd_explore_directory(char* npsd_targetPath, char* npsd_flname, char* npsd_commd) 
{
    DIR* dir;                      // Directory structure
    struct dirent* entry;          // Directory entry structure
    struct stat st;                // File status structure

    if ((dir = opendir(npsd_targetPath)) == NULL) {  // Open the directory
        printf("Errot to open the directory %s\n", npsd_targetPath);
        return;
    }
    //Traversing  directory tree
    while ((entry = readdir(dir)) != NULL)   
    {  
     // Skip "." and ".." directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
        {
            continue;
        }
        // Constructing complete path to the file or directory
        char npsd_entire_path[1024];   
        snprintf(npsd_entire_path, sizeof(npsd_entire_path), "%s/%s", npsd_targetPath, entry->d_name);
         // Checking whether entry is a file or a directory
        if (lstat(npsd_entire_path, &st) == 0) 
        {
            if (S_ISDIR(st.st_mode)) 
            {
                npsd_explore_directory(npsd_entire_path, npsd_flname, npsd_commd);  // Recurse into the subdirectory
            } 
            else if (S_ISREG(st.st_mode)) 
            {
                // Checking whether the file has the desired name
               if (strcmp(entry->d_name, npsd_flname) == 0) {
    time_t c_time = st.st_ctime;
    mode_t permissions = st.st_mode; 
    struct tm *tm_info = localtime(&c_time);

    char perm_str[11]; // String to store file permissions (including null terminator)
    snprintf(perm_str, sizeof(perm_str), "%c%c%c%c%c%c%c%c%c",
             (permissions & S_IRUSR) ? 'r' : '-',
             (permissions & S_IWUSR) ? 'w' : '-',
             (permissions & S_IXUSR) ? 'x' : '-',
             (permissions & S_IRGRP) ? 'r' : '-',
             (permissions & S_IWGRP) ? 'w' : '-',
             (permissions & S_IXGRP) ? 'x' : '-',
             (permissions & S_IROTH) ? 'r' : '-',
             (permissions & S_IWOTH) ? 'w' : '-',
             (permissions & S_IXOTH) ? 'x' : '-');

    strftime(npsd_commd, sizeof(npsd_commd), "%a %d %b %Y %I:%M:%S %p %Z\n", tm_info);
    sprintf(npsd_commd + strlen(npsd_commd), ", \n%ld, %s, %s, %s\n", st.st_size, entry->d_name, npsd_entire_path, perm_str);

    closedir(dir);
    return;
}



            }
        } 
        else 
        {
            printf("Couldn't get information about this file %s\n", npsd_entire_path);
        }
    }

    closedir(dir);  // Closing the directory
}


// Function to check if a date is valid
int npsd_date_input_valid(char* date) 
{
    int npsd_year, npsd_month, npsd_day;  // Year, month, day components

    // Check the format of date YYYY-MM-DD
    if (sscanf(date, "%d-%d-%d", &npsd_year, &npsd_month, &npsd_day) != 3) 
    {
        return 0;
    }
    if (npsd_year < 1 || npsd_year > 9999 || npsd_month < 1 || npsd_month > 12 || npsd_day < 1 || npsd_day > 31) 
    {
        return 0;
    }
    return 1;
}

int check_valid_filelist(char* npsd_fllist)  // Function to check whether a file list is valid or not
{
    if (strlen(npsd_fllist) == 0)  // Checking there is at least one file in the file list
    {
        return 0;
    }
    return 1;
}

int npsd_valid_exten(char* npsd_exten) // Function to check whether extensions list is valid
{
    if (strlen(npsd_exten) == 0) // Checking at least one extension is present in the extensions list
    {
        return 0;
    }
    return 1;
}


// Function to process client requests
void processclient(int sockfd) 
{
    char npsd_buf[1024] = {0};         // Receive buffer
    char npsd_commd[1024] = {0};      // Command buffer
    char npsd_tf[1024] = {0};           // Temporary buffer

    while (1) 
    {
        memset(npsd_buf, 0, sizeof(npsd_buf));
        memset(npsd_commd, 0, sizeof(npsd_commd));
        memcpy(npsd_tf, npsd_buf, sizeof(npsd_buf));

        int npsd_rvalue = read(sockfd, npsd_buf, sizeof(npsd_buf));
        npsd_buf[npsd_rvalue] = '\0';

        char* npsd_token = strtok(npsd_buf, " ");   // Parse command
        if (npsd_token == NULL) 
        {
            sprintf(npsd_commd, "Syntax is not valid. -- Please Enter again.\n");
        } 
        else if (strcmp(npsd_token, "getfn") == 0)  // Execution of getfn command
{ 
    char* npsd_flname = strtok(NULL, " ");
    if (npsd_flname == NULL) {
        sprintf(npsd_commd, "Syntax is not valid. -- Please Enter again.\n");
    } 
    else 
    {
        char npsd_primary_path[1024];
        sprintf(npsd_primary_path, "/home/");  // Search file in the root directory

        char npsd_commd_buf[BUFSIZE]; 
        sprintf(npsd_commd_buf, "find %s -maxdepth 3 -name %s -exec stat --printf='%%s,%%y,%%A\n' {} +",
                npsd_primary_path, npsd_flname);

        FILE* fp = popen(npsd_commd_buf, "r");
        char line[BUFSIZE];
        if (fgets(line, BUFSIZE, fp) != NULL) 
        {
            sprintf(npsd_commd, "%s", line);
        } 
        else 
        {
            sprintf(npsd_commd, "Unable to find the file\n");
        }
        pclose(fp);
    }
}
        // Execution of getfz command

        // getfz command
        /*
        else if (strcmp(npsd_token, "getfz") == 0)  
        { 
            char* npsd_s1_str = strtok(NULL, " ");
            char* npsd_s2_str = strtok(NULL, " ");
            char* npsd_unzip_status = strtok(NULL, " ");

            if (npsd_s1_str == NULL || npsd_s2_str == NULL) 
            {
                sprintf(npsd_commd, "Entered syntax is not valid. Please Enter again.\n");
            } else 
            {
                int size1 = atoi(npsd_s1_str);
                int size2 = atoi(npsd_s2_str);
                if (size1 < 0 || size2 < 0 || size1 > size2) 
                {
                    sprintf(npsd_commd, "Please enter valid size range.\n");
                } else {
                    char npsd_primary_path[1024];
                    sprintf(npsd_primary_path, "/home/");

                    char npsd_commd_buf[BUFSIZE]; // Finding files matching the size range
                    sprintf(npsd_commd_buf, "find %s -type f -size +%dk -size -%dk -print0 | xargs -0 tar -czf temp.tar.gz",
                            npsd_primary_path, size1, size2);
                    system(npsd_commd_buf);

                    if (npsd_unzip_status != NULL && strcmp(npsd_unzip_status, "-u") == 0)  // Check whether the unzip flag is provided
                    {     
                        printf("Unzipping due to uzip flag.\n");
                        system("tar -xzf temp.tar.gz -C /home/raj/backup"); // Unzip the file in the current directory
                         printf("Unzipping DOne\n");
                        sprintf(npsd_commd, "Files are retrieved and has been successfully unzipped.\n");
                    } 
                    else 
                    {
                        sprintf(npsd_commd, "Files are retrieved successfully. Now use -u flag to unzip the files.\n");
                    }
                }
            }
        } 
     */   
        //getfz
        else if (strcmp(npsd_token, "getfz") == 0) {
    char* npsd_s1_str = strtok(NULL, " ");
    char* npsd_s2_str = strtok(NULL, " ");

    if (npsd_s1_str == NULL || npsd_s2_str == NULL) {
        sprintf(npsd_commd, "Entered syntax is not valid. Please enter size1 and size2.\n");
    } else {
        int size1 = atoi(npsd_s1_str);
        int size2 = atoi(npsd_s2_str);

        if (size1 < 0 || size2 < 0 || size1 > size2) {
            sprintf(npsd_commd, "Please enter valid size range.\n");
        } else {
            char npsd_primary_path[1024];
            sprintf(npsd_primary_path, "/home/");

            char npsd_commd_buf[BUFSIZE]; // Finding files matching the size range
           // sprintf(npsd_commd_buf, "find %s -type f -size +%dk -size -%dk -print0 | xargs -0 tar -czf temp.tar.gz",npsd_primary_path, size1, size2);
           //sprintf(npsd_commd_buf, "find %s -type f -size +%dk -size -%dk -print0 | xargs -0 tar -czf /home/raj/backup/temp.tar.gz",npsd_primary_path, size1, size2);
           sprintf(npsd_commd_buf, "find %s -type f -size +%ldc -size -%ldc -print0 | xargs -0 tar -czf /home/raj/backup/temp.tar.gz",npsd_primary_path, size1, size2);

           

            system(npsd_commd_buf);

            sprintf(npsd_commd, "Files matching the size range have been stored in temp.tar.gz\n");
        }
    }
}

        // Execution of getfdb commanf


        // getfdb command
        /*
        else if (strcmp(npsd_token, "getfdb") == 0)  
        {  
            char* npsd_d1_str = strtok(NULL, " ");
            char* npsd_d2_str = strtok(NULL, " ");
            char* npsd_unzip_status = strtok(NULL, " ");

            if (npsd_d1_str == NULL || npsd_d2_str == NULL) {
                sprintf(npsd_commd, "Entered syntax is not valid. Please Enter again.\n");
            } 
            else 
            {
                char npsd_primary_path[1024];
                sprintf(npsd_primary_path, "/home/");

                char npsd_commd_buf[BUFSIZE]; // Finding files that match the date range
                sprintf(npsd_commd_buf, "find %s -type f -newermt \"%s\" ! -newermt \"%s\" -print0 | xargs -0 tar -czf temp.tar.gz",
                        npsd_primary_path, npsd_d1_str, npsd_d2_str);
                system(npsd_commd_buf);

                if (npsd_unzip_status != NULL && strcmp(npsd_unzip_status, "-u") == 0)  // Checking if the unzip flag is provided
                {
                      printf("Unzipping due to uzip flag.\n");
                    system("tar -xzf temp.tar.gz -C /home/raj/backup"); // Unzip the file in the current directory
                      printf("Unzipping DOne\n");
                    sprintf(npsd_commd, "Files are retrieved and has been successfully unzipped.\n");
                } else {
                    sprintf(npsd_commd, "Files are retrieved successfully. Now use -u flag to unzip the files.\n");
                }
            }
        } 
        */
        //getfdb
       else if (strcmp(npsd_token, "getfdb") == 0)
{
    char *npsd_d_str = strtok(NULL, " ");

    if (npsd_d_str == NULL)
    {
        sprintf(npsd_commd, "Entered syntax is not valid. Please Enter again.\n");
    }
    else
    {
        char npsd_primary_path[1024];
        sprintf(npsd_primary_path, "/home/");

        char npsd_commd_buf[BUFSIZE]; // Finding files that match the date range
        sprintf(npsd_commd_buf, "find %s -type f ! -newermt \"%s\" -print0 | xargs -0 tar -czf /home/raj/backup/temp.tar.gz",
                npsd_primary_path, npsd_d_str);
        system(npsd_commd_buf);

        sprintf(npsd_commd, "Files are retrieved successfully.\n");
    }
}

//getfda
else if (strcmp(npsd_token, "getfda") == 0)
{
    char *npsd_d_str = strtok(NULL, " ");

    if (npsd_d_str == NULL)
    {
        sprintf(npsd_commd, "Entered syntax is not valid. Please Enter again.\n");
    }
    else
    {
        char npsd_primary_path[1024];
        sprintf(npsd_primary_path, "/home/");

        char npsd_commd_buf[BUFSIZE]; // Finding files that match the date range
        sprintf(npsd_commd_buf, "find %s -type f -newermt \"%s\" -print0 | xargs -0 tar -czf /home/raj/backup/temp.tar.gz",
                npsd_primary_path, npsd_d_str);
        system(npsd_commd_buf);

        sprintf(npsd_commd, "Files are retrieved successfully.\n");
    }
}




         // Execution of fgets command


         //fgets
        else if (strcmp(npsd_token, "fgets") == 0)
        { 
            char* npsd_fl1 = strtok(NULL, " ");
            char* npsd_fl2 = strtok(NULL, " ");
            char* npsd_fl3 = strtok(NULL, " ");
            char* npsd_fl4 = strtok(NULL, " ");
          

            char npsd_primary_path[1024];
            sprintf(npsd_primary_path, "/home/");

            char npsd_commd_buf[BUFSIZE]; // Check whether any of the specified files are present
            sprintf(npsd_commd_buf, "find %s -type f \\( -iname \"%s\" -o -iname \"%s\" -o -iname \"%s\" -o -iname \"%s\" -o -iname \"%s\" -o -iname \"%s\" \\) -print0 | xargs -0 tar -czf temp.tar.gz",
                    npsd_primary_path, npsd_fl1, npsd_fl2, npsd_fl3, npsd_fl4);
            sprintf(npsd_commd, npsd_commd_buf);
            sprintf(npsd_commd, "%s", npsd_commd_buf);
            int npsd_status = system(npsd_commd_buf);
            // Checking whether the files were found
            if (npsd_status == 0) 
            {
          
               sprintf(npsd_commd, "Files are retrieved successfully. %s %s %s", npsd_fl2, npsd_commd_buf);
                
            } 
            else 
            {
                sprintf(npsd_commd, "File-- not found.\n");
            }
        } 


         // Execution of getft command

         //getft
         /*
        else if (strcmp(npsd_token, "getft") == 0)
        { 
            char* npsd_exten1 = strtok(NULL, " ");
            char* npsd_exten2 = strtok(NULL, " ");
            char* npsd_exten3 = strtok(NULL, " ");
            char* npsd_exten4 = strtok(NULL, " ");
            char* npsd_unzip_status = strtok(NULL, " ");
            int uzip = 0;
            if (strcmp(npsd_exten1, "-u") == 0 || strcmp(npsd_exten2, "-u") == 0 || strcmp(npsd_exten3, "-u") == 0 || strcmp(npsd_exten4, "-u") == 0)
                uzip = 1;
            char npsd_primary_path[1024];
            sprintf(npsd_primary_path, "/home/");

            char npsd_commd_buf[BUFSIZE];  // Checking whether any of the specified files are present
            sprintf(npsd_commd_buf, "find %s -type f \\( ", npsd_primary_path);
            if (npsd_exten1 != NULL) sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten1);
            if (npsd_exten2 != NULL) sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten2);
            if (npsd_exten3 != NULL) sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten3);
            if (npsd_exten4 != NULL) sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten4);
            sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-false \\) -print0 | xargs -0 tar -czf temp.tar.gz");

            int npsd_status = system(npsd_commd_buf);

            if (npsd_status == 0)  // Check if the files were found
            {
                if (strcmp(npsd_tf, "-u") == 0)  // Check whether the unzip flag is provided
                {
                    system("tar -xzf temp.tar.gz -C /home/raj/"); // Unzip the file in the current directory

                    sprintf(npsd_commd, "Files are retrieved and has been successfully unzipped.\n %s %s", npsd_exten2, npsd_commd_buf);
                } else {
                    if(uzip==1)  
                {
                printf("Unzipping due to uzip flag.\n");
               // system("/home/neelpandya/ProjectASP -xzf temp.tar.gz -C /home/neelpandya");
               system("tar -xzf temp.tar.gz -C /home/raj/backup > extraction_log.txt 2>&1");
                printf("Unzipping DOne\n");
               // system("tar -xzf temp.tar.gz -C /home/neelpandya");
                  break;
                }
                else
                        sprintf(npsd_commd, "Files are retrieved successfully. Now use -u flag to unzip the files.\n");
                }
            } 
            else 
            {
                sprintf(npsd_commd, "No file found.\n");
            }

        } 
        */
        //getft
        else if (strcmp(npsd_token, "getft") == 0)
{
    char *npsd_exten1 = strtok(NULL, " ");
    char *npsd_exten2 = strtok(NULL, " ");
    char *npsd_exten3 = strtok(NULL, " ");

    char npsd_primary_path[1024];
    sprintf(npsd_primary_path, "/home/");

    char npsd_commd_buf[BUFSIZE]; // Checking whether any of the specified files are present
    sprintf(npsd_commd_buf, "find %s -type f \\( ", npsd_primary_path);
    if (npsd_exten1 != NULL)
        sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten1);
    if (npsd_exten2 != NULL)
        sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten2);
    if (npsd_exten3 != NULL)
        sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten3);
    sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-false \\) -print0 | xargs -0 tar -czf /home/raj/backup/temp.tar.gz");

    int npsd_status = system(npsd_commd_buf);

    if (npsd_status == 0) // Check if the files were found
    {
        sprintf(npsd_commd, "Files are retrieved successfully.\n");
    }
    else
    {
        sprintf(npsd_commd, "No file found.\n");
    }
}


        //quitc command
        else if (strcmp(npsd_token, "quitc") == 0) 
        {
            sprintf(npsd_commd, "Bye\n");
            break;
        } 
        else 
        {
            sprintf(npsd_commd, "Entered syntax is not valid. Please Enter again.\n");
        }
        send(sockfd, npsd_commd, strlen(npsd_commd), 0); // Send response to client
    }

    close(sockfd);
    exit(0);
}

// Function to redirect client to the mirror server
void npsd_redirecting_to_mirror(int npsd_client_fd) 
{
    char npsd_redirecting_msg[1024];
    snprintf(npsd_redirecting_msg, 1024, "%d\n", MIRROR_PORT);
    send(npsd_client_fd, npsd_redirecting_msg, strlen(npsd_redirecting_msg), 0);
    close(npsd_client_fd);
}

// Main function
int main(int argc, char const* argv[]) 
{
    // Intilizing necessary variables
    int npsd_server_fd, npsd_new_soc;
    struct sockaddr_in address;
    int opt = 1;
    int addrrlen = sizeof(address);
    int npsd_act_clients = 0;

    //SOCKET()
    if ((npsd_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  // Create socket file descriptor
    {
       // Error if socket fails
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(npsd_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) // Attach socket to the port 8080
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //BIND()
    if (bind(npsd_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    //LISTEN()
    if (listen(npsd_server_fd, 3) < 0) 
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    //Infinte Loop 
    while (1) 
    {
        if ((npsd_new_soc = accept(npsd_server_fd, (struct sockaddr*)&address, (socklen_t*)&addrrlen)) < 0) 
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("New client has been connected --->. Forking child process\n");
        if (npsd_act_clients < 6 || (npsd_act_clients > 10 && npsd_act_clients % 2 == 0)) // Load balancing from server to mirror
        { 
            pid_t pid = fork();
            if (pid == -1) 
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
             else if (pid == 0) // Child process
            {  
                close(npsd_server_fd);
                processclient(npsd_new_soc);
            } 
            else // Parent process
            {  
                close(npsd_new_soc);
                while (waitpid(-1, NULL, WNOHANG) > 0); // Clean up zombie processes
            }
        } 
        else 
        {
           // Redirecting to mirror server
            printf("Redirecting to mirror SErver\n"); 
            
            npsd_redirecting_to_mirror(npsd_new_soc);
        }
        npsd_act_clients++; // Count for number of connections
    }

    return 0;
}

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>

#define PORT 7001 // Port number
#define BUFSIZE 1024 // Buffer

// Function to search for a file in directories
void npsd_explore_directory(char *npsd_targetPath, char *npsd_flname, char *npsd_commd)
{
    DIR *dir;                    // Directory structure
    struct dirent *entry;        // Directory entry structure
    struct stat st;              // File status structure

    if ((dir = opendir(npsd_targetPath)) == NULL)
    { // Open the directory
        printf("Not able to open the directory %s\n", npsd_targetPath);
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    { // Traverse the directory tree
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        { // Skip "." and ".." directories
            continue;
        }

        char npsd_entire_path[1024]; // Constructing complete path to the file or directory
        snprintf(npsd_entire_path, sizeof(npsd_entire_path), "%s/%s", npsd_targetPath, entry->d_name);

        if (lstat(npsd_entire_path, &st) == 0)
        { // Checking whether entry is a file or a directory
            if (S_ISDIR(st.st_mode))
            {
                npsd_explore_directory(npsd_entire_path, npsd_flname, npsd_commd); // Recurse into the subdirectory
            }
            else if (S_ISREG(st.st_mode))
            {
                if (strcmp(entry->d_name, npsd_flname) == 0)
                {
                    time_t c_time = st.st_ctime;
                    struct tm *tm_info = localtime(&c_time);

                    char perm_str[11]; // String to store file permissions (including null terminator)
                    snprintf(perm_str, sizeof(perm_str), "%c%c%c%c%c%c%c%c%c",
                             (st.st_mode & S_IRUSR) ? 'r' : '-',
                             (st.st_mode & S_IWUSR) ? 'w' : '-',
                             (st.st_mode & S_IXUSR) ? 'x' : '-',
                             (st.st_mode & S_IRGRP) ? 'r' : '-',
                             (st.st_mode & S_IWGRP) ? 'w' : '-',
                             (st.st_mode & S_IXGRP) ? 'x' : '-',
                             (st.st_mode & S_IROTH) ? 'r' : '-',
                             (st.st_mode & S_IWOTH) ? 'w' : '-',
                             (st.st_mode & S_IXOTH) ? 'x' : '-');

                    sprintf(npsd_commd, "File found: %s\nSize: %ld bytes\nDate Created: %s\nFile Permissions: %s\n", entry->d_name, st.st_size, npsd_commd, perm_str);



                    closedir(dir);
                    return;
                }
            }
            else
            {
                printf("Couldn't get information about this file %s\n", npsd_entire_path);
            }
        }
    }

    closedir(dir); // Closing the directory
}

// Function to process client requests
void processclient(int sockfd)
{
    char npsd_buf[1024] = {0};        // Receive buffer
    char npsd_commd[1024] = {0};      // Command buffer
    char npsd_tf[1024] = {0};          // Temporary buffer

    while (1)
    {
        memset(npsd_buf, 0, sizeof(npsd_buf));
        memset(npsd_commd, 0, sizeof(npsd_commd));
        memcpy(npsd_tf, npsd_buf, sizeof(npsd_buf));

        int npsd_rvalue = read(sockfd, npsd_buf, sizeof(npsd_buf));
        npsd_buf[npsd_rvalue] = '\0';

        char *npsd_token = strtok(npsd_buf, " "); // Parse command
        if (npsd_token == NULL)
        {
            sprintf(npsd_commd, "Entered syntax is not valid. Please Enter again.\n");
        }
        else if (strcmp(npsd_token, "getfn") == 0) // Execution of getfn command
        {
            char *npsd_flname = strtok(NULL, " ");
            if (npsd_flname == NULL)
            {
                sprintf(npsd_commd, "Syntax is not valid. -- Please Enter again.\n");
            }
            else
            {
                char npsd_primary_path[1024];
                sprintf(npsd_primary_path, "/home/"); // Search file in the root directory

                npsd_explore_directory(npsd_primary_path, npsd_flname, npsd_commd);
            }
        }
        // ... (rest of your commands)
        else if (strcmp(npsd_token, "quit") == 0)
        {
            sprintf(npsd_commd, "Bye!!!.\n");
            break;
        }
        else
        {
            sprintf(npsd_commd, "Entered syntax is not valid. Please Enter again.\n");
        }
        send(sockfd, npsd_commd, strlen(npsd_commd), 0); // Send response to client
    }

    close(sockfd);
    exit(0);
}

// Main method for mirror server
int main(int argc, char const* argv[]) 
{
    // Intilizing necessary variables
    int npsd_server_fd, npsd_new_soc;
    struct sockaddr_in address;
    int opt = 1;
    int addrrlen = sizeof(address);
    int npsd_act_clients = 0;

    //SOCKET()
    if ((npsd_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  // Create socket file descriptor
    {
       // Error if socket fails
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(npsd_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) // Attach socket to the port 8080
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //BIND()
    if (bind(npsd_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    //LISTEN()
    if (listen(npsd_server_fd, 3) < 0) 
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    //Infinte Loop 
    while (1) 
    {
        if ((npsd_new_soc = accept(npsd_server_fd, (struct sockaddr*)&address, (socklen_t*)&addrrlen)) < 0) 
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("New client has been connected --->. Forking child process\n");
        if (npsd_act_clients < 6 || (npsd_act_clients > 10 && npsd_act_clients % 2 == 0)) // Load balancing from server to mirror
        { 
            pid_t pid = fork();
            if (pid == -1) 
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
             else if (pid == 0) // Child process
            {  
                close(npsd_server_fd);
                processclient(npsd_new_soc);
            } 
            else // Parent process
            {  
                close(npsd_new_soc);
                while (waitpid(-1, NULL, WNOHANG) > 0); // Clean up zombie processes
            }
        } 
        else 
        {
           // Redirecting to mirror server
            printf("Redirecting to mirror SErver\n"); 
            
            npsd_redirecting_to_mirror(npsd_new_soc);
        }
        npsd_act_clients++; // Count for number of connections
    }

    return 0;
}
*/
