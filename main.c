//
//
//  TemplateMatching
//
//  Created by Michael on 11/30/16.
//  Ugly but it does the do.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <sys/time.h>

#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#define ROTATION_ENABLED

#ifdef ROTATION_ENABLED
#define ROTATION 360
#else
#define ROTATION 1
#endif

typedef struct SumSquareD {
    int SSDr, SSDg, SSDb, sum;
} SSD;

typedef struct imageContainer {
    int x,y,n;
    unsigned char *data;
} image;

typedef struct solutionLocation {
    int x,y;
    SSD colorD;
} sol;

typedef struct coordinatePair {
    int x,y;
} cpair;


void blackPixel (unsigned char * ppointer);
void rotateImage(image * src, image * dest, double angle);
void freeImage(image * img);
int calculateCoordXY(int x, int y, int imageWidth, int imageHeight);
SSD newSSD(void);


SSD newSSD(void) {
    SSD new;
    new.SSDb=0;
    new.SSDg=0;
    new.SSDr=0;
    new.sum=0;
    
    return new;
}


void freeImage(image * img) {
    free(img->data);
}
int calculateCoord(int x, int y, int imageWidth) {
    //offset = (row * NUMCOLS) + column
    return (3*((y*imageWidth)+x));
}
int calculateCoordXY(int x, int y, int imageWidth, int imageHeight) {
    //offset = (row * NUMCOLS) + column
    if ((x*y) > (imageWidth*imageHeight)) return -1;
    if (x > imageWidth || x < 0) return -1;
    if (y > imageHeight || y < 0) return -1;
    
    return (3*((y*imageWidth)+x));
}



sol templateMatch(image search, image template) {
    int lastSSD = INT_MAX;
    SSD colorD;
    
    sol solution = {};
    image templateRotation = {};
    int comparision = 0;
	
	//template rotation
    for (int rotation = 0; rotation < ROTATION; rotation++) {
        rotateImage(&template, &templateRotation, rotation);
        printf("Rotation: %i\n", rotation);

		//cycles through template starting position
        for (int sx = 0; sx <= search.x - template.x; sx++) {
            for (int sy = 0; sy <= search.y - template.y; sy++ ) {
                
                colorD = newSSD();
				//compares template to search image
                for (int tx = 0; tx < templateRotation.x; tx++) {
                    for (int ty = 0; ty < templateRotation.y; ty++) {
                        comparision++;
                        int soffset = calculateCoord(sx+tx, sy+ty, search.x);
                        int toffset = calculateCoord(tx, ty, templateRotation.x);
                        
                        unsigned char *searchPixel = &search.data[soffset];
                        unsigned char *templatePixel = &templateRotation.data[toffset];
                        
                        if (!(*(templatePixel)==0 && *(templatePixel+1)==0 && *(templatePixel+2)==0)) {
                            colorD.SSDb += pow((*searchPixel - *templatePixel), 2);
                            colorD.SSDr += pow((*(searchPixel+1) - *(templatePixel+1)), 2);
                            colorD.SSDg += pow((*(searchPixel+2) - *(templatePixel+2)), 2);
                        }

                    }

                }
                
                colorD.sum = colorD.SSDb + colorD.SSDg + colorD.SSDr;

                if (lastSSD > colorD.sum) {
                    lastSSD = colorD.sum;
                    solution.x = sx;
                    solution.y = sy;
                    solution.colorD = colorD;
                    
                }
                        
            }
        }
        //printf("%i", comparision);
        freeImage(&templateRotation);
    }
    return solution;
}
image drawBox(image search, image template, sol solution) {
    
    image temp = search;
    temp.data = (unsigned char *)malloc(sizeof search.data);
    memcpy(&temp.data, &search.data, sizeof search.data);
    
    for (int x = 0; x < temp.x; x++) {
        for (int y = 0; y < temp.y; y++) {
            int offset = calculateCoord(x, y, temp.x);
            //Draws a box with the middle section cut out
            if ((y+5 > solution.y && y-5 < solution.y+template.y)
                && !(y > solution.y && y < solution.y+template.y)
                && (x+5>solution.x && x-5<solution.x+template.x)) {

                temp.data[offset] = 0;
            }
            
        }
    }
    
    return temp;
}


void copyImage(image * src, image * dest) {
    dest->x = src->x;
    dest->y = src->y;
    dest->n = src->n;
    size_t array_size = sizeof (unsigned char) * src->x * src->y * 3;
    dest->data = malloc(array_size);
    memcpy(dest->data, src->data, array_size);
}

void writeImage(image img, const char *filename) {
    stbi_write_png(filename, img.x, img.y, 3, img.data, 0);
}

void rotateImage(image * src, image * dest, double angle) {
    double radians = angle * M_PI/180;
    double cosR = cos(radians);
    double sinR = sin(radians);
    
    copyImage(src, dest);
    
    int centerx = src->x/2;
    int centery= src->y/2;
    
    for (int x = 0; x < src->x; x++) {
        int m = x - centerx;
        for (int y = 0; y < src->y; y++) {
            int destIndex = calculateCoordXY(x, y, src->x, src->y);
            int n = y - centery;
            int newx = (int) (m * cosR + n * sinR) + centerx;
            int newy = (int) (n * cosR - m * sinR) + centery;
            int sourceIndex = calculateCoordXY(newx, newy, src->x, src->y);
            
            if (sourceIndex == -1) {
                blackPixel(&dest->data[destIndex]);
            } else {
                dest->data[destIndex] = src->data[sourceIndex];
                dest->data[destIndex+1] = src->data[sourceIndex+1];
                dest->data[destIndex+2] = src->data[sourceIndex+2];

            }
        }
    }
    
}

void blackPixel (unsigned char * ppointer) {
    *ppointer = 0;
    *(ppointer+1)=0;
    *(ppointer+2)=0;
}


void resizeImage(image * img, int yAdd, int xAdd) {
    image testImage;
    copyImage(img, &testImage);
    testImage.y = testImage.y + yAdd;
    testImage.x = testImage.x + xAdd;

    size_t array_size = sizeof (unsigned char) * testImage.x * testImage. y* 3;

    
    int originalIndex = img->x * img->y * 3;
    int newIndex = testImage.x * testImage.y * 3;
    
    int y_offset = yAdd/2;
    int x_offset = xAdd/2;
    testImage.data = malloc(array_size);
    memset(testImage.data, 0, array_size);

    for (int x = 0; x <= testImage.x; x++) {
        for (int y = 0; y <= testImage.y; y++) {
       
        int index = calculateCoordXY(x, y, img->x, img->y);
        int newIndex = calculateCoordXY(x+x_offset, y+y_offset, testImage.x, testImage.y);
        if (!(x >= img->x || y >= img->y)) {
            testImage.data[newIndex] = img->data[index];
            testImage.data[newIndex+1] = img->data[index+1];
            testImage.data[newIndex+2] = img->data[index+2];
            }
        }
    }
    
    
    copyImage(&testImage, img);
    freeImage(&testImage);

}
void squareImage(image *img) {
    if (img->x > img->y) {
        resizeImage(img, img->x-img->y, 0);
    }
    if (img->y > img->x) {
        resizeImage(img, 0, img->y-img->x);
    }
}

void runTemplateMatch(const char ** searchFiles, const char ** templateFiles, int numFiles, int channels) {

    for (int i = 0; i < numFiles; i++) {
    
        struct timeval before, after;
        
        gettimeofday(&before, NULL);
        
        image search, template, result;
        search.data = stbi_load(searchFiles[i], &search.x, &search.y, &search.n, 3);
        template.data = stbi_load(templateFiles[i], &template.x, &template.y, &template.n, 3);
        
        
        printf("Template Size: %i, %i, %i\n", template.x, template.y, template.n);
        printf("Search Size: %i, %i, %i\n", search.x, search.y, search.n);
        
        //squareImage(&template);
        //writeImage(template, "outrotate.png");
        sol solution = templateMatch(search, template);
        
        printf("Best Match at: x:%u, y:%u, SSD:%i\n\n", solution.x, solution.y, solution.colorD.sum);
        
        
        result = drawBox(search, template, solution);
        char outputName[256];
        snprintf(outputName, sizeof outputName, "results/output%i.png",i);
        stbi_write_png(outputName, result.x, result.y, channels, result.data, result.x*channels);

        gettimeofday(&after, NULL);
        long time = (after.tv_sec * 1000000 + after.tv_usec)-(before.tv_sec * 1000000 + before.tv_usec);
        printf("Took %ld.%ld seconds\n", time/1000000, time%1000000);

        
    }
    
}


int main(int argc, const char * argv[]) {
    
    image test, rotate;
    test.data = stbi_load("images/license.png", &test.x, &test.y, &test.n, 0);
    squareImage(&test);

    //rotateImage(&test, &rotate, 25);
    //writeImage(test, "test.png");
    
    const char *searchNames[256] = {"images/s1.png", "images/s2.png"};
    const char *templateNames[256] = {"images/t1.png", "images/t2.png"};
    
    int filesToRun = 1; //Use 2 to run both s1 and s2
    
    runTemplateMatch(searchNames, templateNames, filesToRun, 3);
    


    return 0;
}
