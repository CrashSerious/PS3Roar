//
#include <SDL/SDL_ttf.h>
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>

#ifndef __PS3GUI__
#define __PS3GUI__

#define PS3DEBUG
#define blurt	printf ("%s:%s():#%d ::\n", __FILE__, __func__, __LINE__);
TTF_Font *text_fontSmall; 
TTF_Font *text_fontMedium; 
TTF_Font *text_fontLarge; 
TTF_Font *text_fontTiny; 

char * LoadRom(char * path, char * file);
void DrawTTF(int x,int y,char * text, TTF_Font *theFont, SDL_Color Color, SDL_Surface *screen);
void addEgg();
void Populatewindow(char *dir_path, char * selectedfile);
void CreateFile (char *dir_path, char * selectedFile, char * fType);
int listDir (const char *dir_path, char *fileNames[]);
char *path_cat (const char *str1, char *str2);
void redrawBackground();
char inKeys();
void ps3InitDisplay();
void displayStatic(int Egg);

int fileExists(char * fName);
int CheckUSB();
int CreateUSB();
int CopyUSB(char * srcPath, char *dstPath);
int copyFile (char *source, char *dest);
int setPerms(char * fName);

#endif /* __PS3GUI__ */