#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "process.h"
#include "syscall-init.h"
#include "syscall.h"
#include "stdio.h"
#include "memory.h"
#include "dir.h"
#include "fs.h"

void k_thread_a(void *);
void k_thread_b(void *);
void u_prog_a(void);
void u_prog_b(void);

int main(void)
{
   put_str("hello kernel!\n");
   init_all();
   intr_enable();
   process_execute(u_prog_a, "u_prog_a");
   process_execute(u_prog_b, "u_prog_b");
   thread_start("k_thread_a", 31, k_thread_a, "this is thread_a");
   thread_start("k_thread_b", 31, k_thread_b, "this is thread_b");

   int j = 9999999;
   while(j--);

   // create /file1
   int32_t fd = sys_open("/file1", O_CREAT);
   sys_close(fd);

   // open, write and close /file1 twice
   fd = sys_open("/file1", O_RDWR);
   printf("fd:%d\n", fd);
   sys_write(fd, "hello,world\n", 12);
   sys_close(fd);
   fd = sys_open("/file1", O_RDWR);
   sys_write(fd, "hello,world\n", 12);
   sys_close(fd);
   printf("%d closed now\n", fd);

   // read and seek /file1
   fd = sys_open("/file1", O_RDWR);
   printf("open /file1, fd:%d\n", fd);
   char buf[64] = {0};
   int32_t read_bytes = sys_read(fd, buf, 18);
   printf("1_ read %d bytes:\n%s\n", read_bytes, buf);
   memset(buf, 0, 64);
   read_bytes = sys_read(fd, buf, 6);
   printf("2_ read %d bytes:\n%s", read_bytes, buf);
   memset(buf, 0, 64);
   read_bytes = sys_read(fd, buf, 6);
   printf("3_ read %d bytes:\n%s", read_bytes, buf);
   printf("________  close file1 and reopen  ________\n");
   sys_close(fd);
   fd = sys_open("/file1", O_RDWR);
   memset(buf, 0, 64);
   read_bytes = sys_read(fd, buf, 24);
   printf("4_ read %d bytes:\n%s", read_bytes, buf);
   // sys_close(fd);
   printf("________  SEEK_SET 0  ________\n");
   sys_lseek(fd, 0, SEEK_SET);
   memset(buf, 0, 64);
   read_bytes = sys_read(fd, buf, 24);
   printf("5_ read %d bytes:\n%s", read_bytes, buf);

   // delete /file1
   printf("/file1 delete %s!\n", sys_unlink("/file1") == 0 ? "done" : "fail");

   j = 9999999;
   while(j--);

   // mkdir /dir1/subdir1 and corw /dir1/subdir1/file2
   printf("/dir1/subdir1 create %s!\n", sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
   printf("/dir1 create %s!\n", sys_mkdir("/dir1") == 0 ? "done" : "fail");
   printf("now, /dir1/subdir1 create %s!\n", sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
   fd = sys_open("/dir1/subdir1/file2", O_CREAT | O_RDWR);
   if (fd != -1)
   {
      printf("/dir1/subdir1/file2 create done!\n");
      sys_write(fd, "Catch me if you can!\n", 21);
      sys_lseek(fd, 0, SEEK_SET);
      char buf[32] = {0};
      sys_read(fd, buf, 21);
      printf("/dir1/subdir1/file2 says:\n%s", buf);
      sys_close(fd);
   }

   // open and close /dir1/subdir1
   struct dir *p_dir = sys_opendir("/dir1/subdir1");
   if (p_dir)
   {
      printf("/dir1/subdir1 open done!\ncontent:\n");
      char *type = NULL;
      struct dir_entry *dir_e = NULL;
      while ((dir_e = sys_readdir(p_dir)))
      {
         if (dir_e->f_type == FT_REGULAR)
         {
            type = "regular";
         }
         else
         {
            type = "directory";
         }
         printf("      %s   %s\n", type, dir_e->filename);
      }
      if (sys_closedir(p_dir) == 0)
      {
         printf("/dir1/subdir1 close done!\n");
      }
      else
      {
         printf("/dir1/subdir1 close fail!\n");
      }
   }
   else
   {
      printf("/dir1/subdir1 open fail!\n");
   }

   // delete /dir1/subdir1
   printf("/dir1 content before delete /dir1/subdir1:\n");
   struct dir *dir = sys_opendir("/dir1/");
   char *type = NULL;
   struct dir_entry *dir_e = NULL;
   while ((dir_e = sys_readdir(dir)))
   {
      if (dir_e->f_type == FT_REGULAR)
      {
         type = "regular";
      }
      else
      {
         type = "directory";
      }
      printf("      %s   %s\n", type, dir_e->filename);
   }
   printf("try to delete nonempty directory /dir1/subdir1\n");
   if (sys_rmdir("/dir1/subdir1") == -1)
   {
      printf("sys_rmdir: /dir1/subdir1 delete fail!\n");
   }
   printf("try to delete /dir1/subdir1/file2\n");
   if (sys_rmdir("/dir1/subdir1/file2") == -1)
   {
      printf("sys_rmdir: /dir1/subdir1/file2 delete fail!\n");
   }
   if (sys_unlink("/dir1/subdir1/file2") == 0)
   {
      printf("sys_unlink: /dir1/subdir1/file2 delete done\n");
   }
   printf("try to delete directory /dir1/subdir1 again\n");
   if (sys_rmdir("/dir1/subdir1") == 0)
   {
      printf("/dir1/subdir1 delete done!\n");
   }
   printf("/dir1 content after delete /dir1/subdir1:\n");
   sys_rewinddir(dir);
   while ((dir_e = sys_readdir(dir)))
   {
      if (dir_e->f_type == FT_REGULAR)
      {
         type = "regular";
      }
      else
      {
         type = "directory";
      }
      printf("      %s   %s\n", type, dir_e->filename);
   }

   // pwd
   char cwd_buf[32] = {0};
   sys_getcwd(cwd_buf, 32);
   printf("cwd:%s\n", cwd_buf);
   sys_chdir("/dir1");
   printf("change cwd now\n");
   sys_getcwd(cwd_buf, 32);
   printf("cwd:%s\n", cwd_buf);

   // stat
   struct stat obj_stat;
   sys_stat("/", &obj_stat);
   printf("/`s info\n   i_no:%d\n   size:%d\n   filetype:%s\n",
          obj_stat.st_ino, obj_stat.st_size,
          obj_stat.st_filetype == 2 ? "directory" : "regular");
   sys_stat("/dir1", &obj_stat);
   printf("/dir1`s info\n   i_no:%d\n   size:%d\n   filetype:%s\n",
          obj_stat.st_ino, obj_stat.st_size,
          obj_stat.st_filetype == 2 ? "directory" : "regular");

   while (1)
      ;
   return 0;
}

void k_thread_a(void *arg)
{
   void *addr1 = sys_malloc(256);
   void *addr2 = sys_malloc(255);
   void *addr3 = sys_malloc(254);
   console_put_str(" thread_a malloc addr:0x");
   console_put_int((int)addr1);
   console_put_char(',');
   console_put_int((int)addr2);
   console_put_char(',');
   console_put_int((int)addr3);
   console_put_char('\n');

   int cpu_delay = 9999999;
   while (cpu_delay-- > 0)
      ;
   sys_free(addr1);
   sys_free(addr2);
   sys_free(addr3);
   while (1)
      ;
}

void k_thread_b(void *arg)
{
   void *addr1 = sys_malloc(256);
   void *addr2 = sys_malloc(255);
   void *addr3 = sys_malloc(254);
   console_put_str(" thread_b malloc addr:0x");
   console_put_int((int)addr1);
   console_put_char(',');
   console_put_int((int)addr2);
   console_put_char(',');
   console_put_int((int)addr3);
   console_put_char('\n');

   int cpu_delay = 999999;
   while (cpu_delay-- > 0)
      ;
   sys_free(addr1);
   sys_free(addr2);
   sys_free(addr3);
   while (1)
      ;
}

void u_prog_a(void)
{
   void *addr1 = malloc(256);
   void *addr2 = malloc(255);
   void *addr3 = malloc(254);
   printf(" prog_a malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);

   int cpu_delay = 100000;
   while (cpu_delay-- > 0)
      ;
   free(addr1);
   free(addr2);
   free(addr3);
   while (1)
      ;
}

void u_prog_b(void)
{
   void *addr1 = malloc(256);
   void *addr2 = malloc(255);
   void *addr3 = malloc(254);
   printf(" prog_b malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);

   int cpu_delay = 100000;
   while (cpu_delay-- > 0)
      ;
   free(addr1);
   free(addr2);
   free(addr3);
   while (1)
      ;
}