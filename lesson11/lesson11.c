/*
 * This code was created by Jeff Molofee '99 (ported to Linux/GLUT by Richard Campbell '99)
 *
 * If you've found this code useful, please let me know.
 *
 * Visit Jeff Molofee at www.demonews.com/hosted/nehe
 * (email Richard Campbell at ulmont@bellsouth.net)
 *
 * Modified by Hayaki Saito (user@zuse.jp) for libsixel integration.
 */

#include "config.h"

#if HAVE_OSMESA
# define USE_OSMESA 1
#elif HAVE_CGL
# define USE_CGL 1
#elif HAVE_X11
# define USE_GLX 1
#else
# error
#endif

#if USE_OSMESA
# include <GL/osmesa.h>
#elif USE_CGL
# include <OpenGL/gl.h>
# include <OpenGL/OpenGL.h>
#elif USE_GLX
# include <X11/Xlib.h>
# include <GL/glx.h>
# include <GL/gl.h>
#endif

#include <stdio.h>      /* Header file for standard file i/o */
#include <stdlib.h>     /* Header file for malloc/free */
#include <memory.h>     /* Header file for memcpy */
#include <unistd.h>     /* needed to sleep */
#include <math.h>       /* for sin function */
#include <sixel.h>      /* libsixel */

#if HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#if HAVE_TERMIOS_H
# include <termios.h>
#endif
#if HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifndef M_PI
# define M_PI 3.1415926535897932386
#endif

#if USE_OSMESA
static unsigned char *pbuffer;
static OSMesaContext context;
#elif USE_CGL
static CGLPBufferObj pbuffer;
static CGLContextObj context;
#elif USE_GLX && (defined(GLX_VERSION_1_3) || defined(GLX_VERSION_1_4))
static Display *display = NULL;
static GLXPbuffer pbuffer = 0;
static GLXContext context;
#endif
static volatile int signaled = 0;

static void sighandler(int sig)
{
    signaled = sig;
}

static int Setup(int width, int height)
{
#if USE_OSMESA
    const size_t size = width * height * 4;
    pbuffer = malloc(size);
    context = OSMesaCreateContextExt(GL_RGBA, 24, 0, 0, 0);
    OSMesaMakeCurrent(context, (void *)pbuffer, GL_UNSIGNED_BYTE, width, height);
    return 0;
#elif USE_CGL
    /* OpenGL PBuffer initialization: OSX specific */
    CGLPixelFormatAttribute pfattr[] = {
        kCGLPFAPBuffer,
        (CGLPixelFormatAttribute)0
    };
    CGLPixelFormatObj pixformat;
    GLint npixels;
    int e;

    e = CGLChoosePixelFormat(pfattr, &pixformat, &npixels);
    if (e != kCGLNoError) {
       fprintf(stderr, "CGLChoosePixelFormat failed, err %d\n", e);
       return e;
    }
    e = CGLCreateContext(pixformat, 0, &context);
    if (e != kCGLNoError) {
       fprintf(stderr, "CGLChoosePixelFormat failed, err %d\n", e);
       return e;
    }
    e = CGLDestroyPixelFormat(pixformat);
    if (e != kCGLNoError) {
       fprintf(stderr, "CGLDestroyPixelFormat failed, err %d\n", e);
       return e;
    }
    e = CGLSetCurrentContext(context);
    if (e != kCGLNoError) {
       fprintf(stderr, "CGLSetCurrentContext failed, err %d\n", e);
       return e;
    }
    e = CGLCreatePBuffer(width, height, GL_TEXTURE_2D, GL_RGB, 0, &pbuffer);
    if (e != kCGLNoError) {
       fprintf(stderr, "CGLCreatePBuffer failed, err %d\n", e);
       return e;
    }
    e = CGLSetPBuffer(context, pbuffer, 0, 0, 0);
    if (e != kCGLNoError) {
       fprintf(stderr, "CGLSetPBuffer failed, err %d\n", e);
       return e;
    }
    return kCGLNoError;
#elif USE_GLX
    /* Open the X display */
    display = XOpenDisplay(NULL);
    if (!display) {
       printf("Error: couldn't open default X display.\n");
       return (-1);
    }

    /* Get default screen */
    int screen = DefaultScreen(display);

    char *glxversion;

    glxversion = (char *) glXGetClientString(display, GLX_VERSION);
    if (!(strstr(glxversion, "1.3") || strstr(glxversion, "1.4"))) {
       XCloseDisplay(display);
       return (-1);
    }

    glxversion = (char *) glXQueryServerString(display, screen, GLX_VERSION);
    if (!(strstr(glxversion, "1.3") || strstr(glxversion, "1.4"))) {
       XCloseDisplay(display);
       return (-1);
    }

    /* Create Pbuffer */
    GLXFBConfig *fbConfigs;
    GLXFBConfig chosenFBConfig;
    GLXFBConfig fbconfig = 0;
    GLXPbuffer pbuffer = None;

    int nConfigs;
    int fbconfigid;

    int fbAttribs[] = {
       GLX_RENDER_TYPE, GLX_RGBA_BIT,
       GLX_DEPTH_SIZE, 1,
       GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT | GLX_PBUFFER_BIT,
       None
    };

    int pbAttribs[] = {
       GLX_PBUFFER_WIDTH, 0,
       GLX_PBUFFER_HEIGHT, 0,
       GLX_LARGEST_PBUFFER, False,
       GLX_PRESERVED_CONTENTS, False,
       None
    };

    pbAttribs[1] = width;
    pbAttribs[3] = height;

    fbConfigs = glXChooseFBConfig(display, screen, fbAttribs, &nConfigs);

    if (0 == nConfigs || !fbConfigs) {
       printf("Error: glxChooseFBConfig failed\n");
       XFree(fbConfigs);
       XCloseDisplay(display);
       return (-1);
    }

    chosenFBConfig = fbConfigs[0];

    glXGetFBConfigAttrib(display, chosenFBConfig, GLX_FBCONFIG_ID, &fbconfigid);
    printf("Chose 0x%x as fbconfigid\n", fbconfigid);

    /* Create the pbuffer using first fbConfig in the list that works. */
    pbuffer = glXCreatePbuffer(display, chosenFBConfig, pbAttribs);

    if (pbuffer) {
       fbconfig = chosenFBConfig;
    }

    XFree(fbConfigs);

    if (pbuffer == None) {
       printf("Error: couldn't create pbuffer\n");
       XCloseDisplay(display);
       return (-1);
    }

    /* Create GLX context */
    context = glXCreateNewContext(display, fbconfig, GLX_RGBA_TYPE, NULL, True);
    if (context) {
       if (!glXIsDirect(display, context)) {
          printf("Warning: using indirect GLXContext\n");
       }
    }
    else {
       printf("Error: Couldn't create GLXContext\n");
       XCloseDisplay(display);
       return (-1);
    }

    /* Bind context to pbuffer */
    if (!glXMakeCurrent(display, pbuffer, context)) {
       printf("Error: glXMakeCurrent failed\n");
       XCloseDisplay(display);
       return (-1);
    }
    return 0;
#else
    /* TODO: pbuffer initialization */
    return 0;
#endif
}

static int
cleanup(void)
{
#if USE_OSMESA
    OSMesaDestroyContext(context);
    free(pbuffer);
#elif USE_CGL
    (void)CGLDestroyContext(context);
    (void)CGLDestroyPBuffer(pbuffer);
#elif USE_GLX
    display = XOpenDisplay(NULL);
    glXDestroyPbuffer(display, pbuffer);
    XCloseDisplay(display);
#else
    /* TODO: cleanup pbuffer and OpenGL context */
#endif
    return 0;
}


static int
sixel_write(char *data, int size, void *priv)
{
    return fwrite(data, 1, size, (FILE *)priv);
}

static int
OutputSixel(unsigned char *pixbuf, int width, int height,
             int ncolors, int depth)
{
    sixel_output_t *context;
    sixel_dither_t *dither;
    int ret;

    context = sixel_output_create(sixel_write, stdout);
    dither = sixel_dither_create(ncolors);
#if USE_OSMESA
    sixel_dither_set_pixelformat(dither, PIXELFORMAT_RGBA8888);
#endif
    ret = sixel_dither_initialize(dither, pixbuf, width, height, depth,
                                  LARGE_AUTO, REP_AUTO, QUALITY_AUTO);
    if (ret != 0)
        return ret;
    ret = sixel_encode(pixbuf, width, height, depth, dither, context);
    if (ret != 0)
        return ret;
    sixel_output_unref(context);
    sixel_dither_unref(dither);

    return 0;
}


static int
Wait(int usec)
{
#if HAVE_SYS_SELECT_H
    fd_set rfds;
    struct timeval tv;
#endif  /* HAVE_SYS_SELECT_H */
    int ret = 0;

#if HAVE_SYS_SELECT_H
    tv.tv_sec = usec / 1000000;
    tv.tv_usec = usec % 1000000;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    ret = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
#else
    (void) usec;
#endif  /* HAVE_SYS_SELECT_H */

    return ret;
}

static void Scroll(int pixelheight)
{
#if HAVE_SYS_IOCTL_H
    struct winsize size = {0, 0, 0, 0};
#endif
#if HAVE_TERMIOS_H
    struct termios old_termios;
    struct termios new_termios;
#endif
    int row = 0;
    int col = 0;
    int cellheight;
    int scroll;

#if HAVE_SYS_IOCTL_H
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    if (size.ws_ypixel <= 0) {
        printf("\033[H\0337");
        return;
    }
# if HAVE_TERMIOS_H
    /* set the terminal to cbreak mode */
    tcgetattr(STDIN_FILENO, &old_termios);
    memcpy(&new_termios, &old_termios, sizeof(old_termios));
    new_termios.c_lflag &= ~(ECHO | ICANON);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);

    /* request cursor position report */
    printf("\033[6n");
    if (Wait(1000 * 1000) != (-1)) { /* wait 1 sec */
        if (scanf("\033[%d;%dR", &row, &col) == 2) {
            cellheight = pixelheight * size.ws_row / size.ws_ypixel + 1;
            scroll = cellheight + row - size.ws_row;
            printf("\033[%dS\033[%dA", scroll, scroll);
            printf("\0337");
        } else {
            printf("\033[H\0337");
        }
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios);
# else
    printf("\033[H\0337");
# endif  /* HAVE_TERMIOS_H */
#else
    printf("\033[H\0337");
#endif  /* HAVE_SYS_IOCTL_H */
}

/* floats for x rotation, y rotation, z rotation */
float xrot, yrot, zrot;
float points[45][45][3];   /* the array for the points on the grid of our "wave" */
int wiggle_count = 0;

/* storage for one texture  */
GLuint texture[1];

/* Image type - contains height, width, and data */
typedef struct _Image {
    uint32_t sizeX;
    uint32_t sizeY;
    char *data;
} Image;

/* quick and dirty bitmap loader...for 24 bit bitmaps with 1 plane only.
 * See http://www.dcs.ed.ac.uk/~mxr/gfx/2d/BMP.txt for more info.
 */
int ImageLoad(char *filename, Image *image)
{
    FILE *file;
    uint32_t size;            /* size of the image in bytes. */
    uint32_t i;               /* standard counter. */
    uint16_t planes;          /* number of planes in image (must be 1) */
    uint16_t bpp;             /* number of bits per pixel (must be 24) */
    char temp;                /* temporary color storage for bgr-rgb conversion. */

    /* make sure the file is there. */
    if ((file = fopen(filename, "rb"))==NULL)
    {
        printf("File Not Found : %s\n",filename);
        return 0;
    }

    /* seek through the bmp header, up to the width/height: */
    fseek(file, 18, SEEK_CUR);

    /* read the width */
    if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
        printf("Error reading width from %s.\n", filename);
        return 0;
    }
    printf("Width of %s: %u\n", filename, image->sizeX);

    /* read the height */
    if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
        printf("Error reading height from %s.\n", filename);
        return 0;
    }
    printf("Height of %s: %u\n", filename, image->sizeY);

    /* calculate the size (assuming 24 bits or 3 bytes per pixel). */
    size = image->sizeX * image->sizeY * 3;

    /* read the planes */
    if ((fread(&planes, 2, 1, file)) != 1) {
        printf("Error reading planes from %s.\n", filename);
        return 0;
    }
    if (planes != 1) {
        printf("Planes from %s is not 1: %u\n", filename, planes);
        return 0;
    }

    /* read the bpp */
    if ((i = fread(&bpp, 2, 1, file)) != 1) {
        printf("Error reading bpp from %s.\n", filename);
        return 0;
    }
    if (bpp != 24) {
        printf("Bpp from %s is not 24: %u\n", filename, bpp);
        return 0;
    }

    /* seek past the rest of the bitmap header. */
    fseek(file, 24, SEEK_CUR);

    /* read the data. */
    image->data = (char *) malloc(size);
    if (image->data == NULL) {
        printf("Error allocating memory for color-corrected image data");
        return 0;	
    }

    if ((i = fread(image->data, size, 1, file)) != 1) {
        printf("Error reading image data from %s.\n", filename);
        return 0;
    }

    for (i = 0; i < size; i += 3) { /* reverse all of the colors. (bgr -> rgb) */
        temp = image->data[i];
        image->data[i] = image->data[i + 2];
        image->data[i+2] = temp;
    }

    /* we're done. */
    return 1;
}

/* Load Bitmaps And Convert To Textures */
void LoadGLTextures()
{	
    /* Load Texture */
    Image *image1;

    /* allocate space for texture */
    image1 = (Image *) malloc(sizeof(Image));
    if (image1 == NULL) {
        printf("Error allocating space for image");
        exit(0);
    }

    if (!ImageLoad("tim.bmp", image1)) {
        exit(1);
    }

    /* Create Texture */
    glGenTextures(1, &texture[0]);
    glBindTexture(GL_TEXTURE_2D, texture[0]);   /* 2d texture (x and y size) */

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); /* scale linearly when image bigger than texture */
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); /* scale linearly when image smalled than texture */

    /* 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image,
     * border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
     */
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);
};

/* A general OpenGL initialization function.  Sets all of the initial parameters. */
void InitGL(int width, int height)	        /* We call this right after our OpenGL window is created. */
{
    glShadeModel(GL_SMOOTH);			/* Enables Smooth Color Shading */
    LoadGLTextures();				/* Load The Texture(s) */
    glEnable(GL_TEXTURE_2D);			/* Enable Texture Mapping */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	/* Clear The Background Color To Blue */
    glClearDepth(1.0);				/* Enables Clearing Of The Depth Buffer */
    glDepthFunc(GL_LESS);			/* The Type Of Depth Test To Do */
    glEnable(GL_DEPTH_TEST);			/* Enables Depth Testing */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();				/* Reset The Projection Matrix */
    GLfloat fovy = 45;
    GLfloat aspect = (GLfloat)width / (GLfloat)height;
    GLfloat znear = 0.1;
    GLfloat zfar = 100;
    GLfloat radian= 2 * M_PI * fovy / 360.0;
    GLfloat t = (GLfloat)(1.0 / tan(radian / 2));
    GLfloat matrix[]={
        t / aspect, 0, 0, 0,
        0, t, 0, 0,
        0, 0, (zfar + znear) / (znear - zfar), -1,
        0, 0, (2 * zfar * znear) / (znear - zfar), 0
    };
    glLoadMatrixf(matrix);

    glMatrixMode(GL_MODELVIEW);

    float float_x, float_y;                     /* loop counters */
    for(float_x = 0.0f; float_x < 9.0f; float_x +=  0.2f )	{
        for(float_y = 0.0f; float_y < 9.0f; float_y += 0.2f)		{
            points[(int)(float_x * 5)][(int)(float_y * 5)][0] = float_x - 4.4f;
            points[(int)(float_x * 5)][(int)(float_y * 5)][1] = float_y - 4.4f;
            points[(int)(float_x * 5)][(int)(float_y * 5)][2] = (float)(sin(((float_x * 5 * 8) / 360) * 3.14159 * 2));
        }
    }
}

/* The main drawing function. */
static void
DrawGLScene(void)
{
    int x, y;
    float float_x, float_y, float_xb, float_yb;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		/* Clear The Screen And The Depth Buffer */
    glLoadIdentity();				/* Reset The View */
    glRotatef(60.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(30.0f, 0.0f, 0.0f, 1.0f);
    glTranslatef(-8.0f, -14.0f, -8.0f);

    glBindTexture(GL_TEXTURE_2D, texture[0]);   /* choose the texture to use. */

    glPolygonMode(GL_BACK, GL_FILL);
    glPolygonMode(GL_FRONT, GL_LINE);

    glBegin(GL_QUADS);
    for (x = 0; x < 44; x++) {
        for (y = 0; y < 44; y++) {
            float_x  = (float) (x) /44;
            float_y  = (float) (y) /44;
            float_xb = (float) (x+1) /44;
            float_yb = (float) (y+1) /44;

            glTexCoord2f( float_x, float_y);
            glVertex3f( points[x][y][0], points[x][y][1], points[x][y][2] );
        		
            glTexCoord2f( float_x, float_yb );
            glVertex3f( points[x][y+1][0], points[x][y+1][1], points[x][y+1][2] );
        		
            glTexCoord2f( float_xb, float_yb );
            glVertex3f( points[x+1][y+1][0], points[x+1][y+1][1], points[x+1][y+1][2] );
        		
            glTexCoord2f( float_xb, float_y );
            glVertex3f( points[x+1][y][0], points[x+1][y][1], points[x+1][y][2] );
        }
    }
    glEnd();

    if (wiggle_count == 2) { /* cycle the sine values */
        for (y = 0; y < 45; y++) {
            points[44][y][2] = points[0][y][2];
        }

        for( x = 0; x < 44; x++ ) {
            for( y = 0; y < 45; y++) {
        	points[x][y][2] = points[x+1][y][2];
            }		
        }		
        wiggle_count = 0;
    }
    wiggle_count++;

    xrot +=0.3f;
    yrot +=0.2f;
    zrot +=0.4f;
}

int main(int argc, char **argv)
{
    unsigned char *pixbuf;

    int width = 400;
    int height = 300;
    int ncolors = 16;

    (void) argc;
    (void) argv;

    if (signal(SIGINT, sighandler) == SIG_ERR)
       return (-1);
    if (Setup(width, height) != 0)
       return (-1);

    InitGL(width, height);
    printf("\033[?25l");
    Scroll(height);

#if !defined(USE_OSMESA)
    pixbuf = malloc(width * height * 3);
#endif

    while (!signaled) {
        glPushMatrix();
        glScalef(1, -1, 1);
        DrawGLScene();
        glPopMatrix();

        if (signaled)
            break;

        printf("\0338");
#if USE_OSMESA
        pixbuf = pbuffer;
#else
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixbuf);
#endif
        OutputSixel(pixbuf, width, height, ncolors, /* unused */ 3);
    }

#if !defined(USE_OSMESA)
    free(pixbuf);
#endif
    printf("\033[?25h");
    printf("\030\033\\");

    if (cleanup() != 0)
       return (-1);
    return 0;
}

/* EOF */
