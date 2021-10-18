#include <Arduino.h>
#include "FS.h"
#include <LITTLEFS.h>
#include "cmdArduino.h"
#ifndef CONFIG_LITTLEFS_FOR_IDF_3_2
 #include <time.h>
#endif
#define FORMAT_LITTLEFS_IF_FAILED true

void listDir(int argCnt, char **args){
    
    const char * dirname = args[1] ;
    uint8_t levels =cmd.conv(args[2], 10);

    Serial.printf("Listing directory: %s\r\n", dirname);
    File root = LITTLEFS.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
           
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(int argCnt, char **args){
    const char * path = args[1];
    Serial.printf("Creating Dir: %s\n", path);
    if(LITTLEFS.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(int argCnt, char **args){
    const char * path = args[1];
    Serial.printf("Removing Dir: %s\n", path);
    if(LITTLEFS.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(int argCnt, char **args){
     const char * path = args[1];
    Serial.printf("Reading file: %s\r\n", path);

    File file = LITTLEFS.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(int argCnt, char **args){
    const char * path = args[1];
    const char * message = args[2];
    Serial.printf("Writing file: %s\r\n", path);

    File file = LITTLEFS.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(int argCnt, char **args){
    const char * path = args[1];
    const char * message = args[2];
    Serial.printf("Appending to file: %s\r\n", path);
    File file = LITTLEFS.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

void renameFile(int argCnt, char **args){
    const char * path1 = args[1];
    const char * path2 = args[2];
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (LITTLEFS.rename(path1, path2)) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}

void deleteFile(int argCnt, char **args){
    const char * path = args[1];
    Serial.printf("Deleting file: %s\r\n", path);
    if(LITTLEFS.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

// SPIFFS-like write and delete file, better use #define CONFIG_LITTLEFS_SPIFFS_COMPAT 1

void writeFile2(int argCnt, char **args){
    const char * path = args[1];
    const char * message =args[2];

    if(!LITTLEFS.exists(path)){
		if (strchr(path, '/')) {
            Serial.printf("Create missing folders of: %s\r\n", path);
			char *pathStr = strdup(path);
			if (pathStr) {
				char *ptr = strchr(pathStr, '/');
				while (ptr) {
					*ptr = 0;
					LITTLEFS.mkdir(pathStr);
					*ptr = '/';
					ptr = strchr(ptr+1, '/');
				}
			}
			free(pathStr);
		}
    }

    Serial.printf("Writing file to: %s\r\n", path);
    File file = LITTLEFS.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void deleteFile2(int argCnt, char **args){
    const char * path = args[1];
    Serial.printf("Deleting file and empty folders on path: %s\r\n", path);

    if(LITTLEFS.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }

    char *pathStr = strdup(path);
    if (pathStr) {
        char *ptr = strrchr(pathStr, '/');
        if (ptr) {
            Serial.printf("Removing all empty folders on path: %s\r\n", path);
        }
        while (ptr) {
            *ptr = 0;
            LITTLEFS.rmdir(pathStr);
            ptr = strrchr(pathStr, '/');
        }
        free(pathStr);
    }
}

void testFileIO(int argCnt, char **args){
    const char * path = args[1];
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = LITTLEFS.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = LITTLEFS.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("- failed to open file for reading");
    }
}

void setup(){
    cmd.begin(115200);
    Serial.begin(115200);
    if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LITTLEFS Mount Failed");
        return;
    }
    cmd.add("ls",listDir);  // cach dung : ls path 0  
    cmd.add("mkdir",createDir);// mkdir path 
    cmd.add("rmdir",removeDir);// rmdir path 
    cmd.add("readfile",readFile);// readfile path 
    cmd.add("writefile",writeFile);// writefile path messenger  
    cmd.add("renamefile",renameFile); // renamefile path1 path2 
    cmd.add("deletefile",deleteFile); // deletefile path 
    cmd.add("writefile2",writeFile2); // writefile2 path 
    cmd.add("deletefile2",deleteFile2);// deletefile2 path 
    cmd.add("testfileio",testFileIO); // tesstfileio path 
    
    
	
	
    Serial.println( "Test complete" );
}

void loop(){
   cmd.poll();
}
