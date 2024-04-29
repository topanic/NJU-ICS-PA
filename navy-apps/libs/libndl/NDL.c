#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>

#define IS_NUM(ch) (ch >= '0' && ch <= '9')
static int canvas_w = 0, canvas_h = 0;

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;

uint32_t NDL_GetTicks() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // 返回毫秒数
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  int ret = read(evtdev, buf, len);
  return ret == 0 ? 0 : 1;
}

void NDL_OpenCanvas(int *w, int *h) {
    if (*w == 0 && *h == 0) {
        *w = canvas_w;
        *h = canvas_h;
    }
    assert(*w <= canvas_w);
    assert(*h <= canvas_h);
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
}

#define END_LEN 0

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
    // 在显示器坐标为(x, y)写入宽度w，高度height，像素为pixels的图片
    assert(w > 0 && w <= canvas_w);
    assert(h > 0 && h <= canvas_h);
    // 写入显示器
    for (size_t j = 0; j < h; ++j) {
        lseek(fbdev, (y + j) * canvas_w + x, SEEK_SET);
        write(fbdev, pixels + j * w, w);
    }
    // 最后提醒写入结束
    write(fbdev, 0, END_LEN);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
    if (getenv("NWM_APP")) {
        evtdev = 3;
    }
    // open 3 dev file
    evtdev = open("/dev/events", 0, 0);
    fbdev = open("/dev/fb", 0, 0);
    FILE *fp = fopen("/proc/dispinfo", "r");

    char buf[20];
    int n = 8;
    fscanf(fp, "WIDTH:%d\nHEIGHT:%d\n", &canvas_w, &canvas_h);
    fclose(fp);
    // printf("width:%d, height:%d\n", canvas_w, canvas_h);
    return 0;
}

void NDL_Quit() {
    close(evtdev);
    close(fbdev);
}
