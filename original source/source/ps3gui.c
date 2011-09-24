#include "ps3gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <string.h>

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_mixer.h>
#include <io/pad.h>
#include <sysutil/video.h>
#include <rsx/gcm_sys.h>
#include <rsx/rsx.h>
#include <sys/systime.h>
#include "xroar.h"
#include "rsxutil.h"

extern SDL_Surface *screen;
extern SDL_Surface *backgroundScreen;
extern SDL_Surface *egg;
extern const char *ROMSdir;
extern const char *CASdir;
extern const char *DSKdir;
extern const char *SNAPdir;
extern const char *IMAGEdir;
extern const char *SOUNDdir;
extern const char *BIOSdir;
extern const char *USBPS3RoarDir;
extern const char *USBROMSdir;
extern const char *USBCASdir;
extern const char *USBDSKdir;
extern const char *USBSNAPdir;
extern const char *USBIMAGEdir;
extern const char *USBSOUNDdir;
extern const char *USBBIOSdir;
int direction = 3;

SDL_Color Red = {255,0,0,0};  // Red
SDL_Color Green = {0,255,0,0};  // Green
SDL_Color Blue = {0,0,255,0};  // Blue

SDL_Surface *staticScreen1, *staticScreen2, *staticScreen3, *scroller1, *testPattern1, *openDialog, *saveDialog;
char* static1 = "static1.png";
char* static2 = "static2.png";
char* static3 = "static3.png";
char* strscroller1 = "scroller1.png";
char* strtestPattern1 = "test_pattern.png";
char* strOpenDialog = "open.png";
char* strSaveDialog = "entry.png";

/* Rectangle for the TV Screen */
SDL_Rect tvScreen;
SDL_Rect CRTRect;
SDL_Rect scrollerRect;
SDL_Rect tickerRect;

int fileExists (char * fName)
{
	/* returns 1 if the file exists an 0 if it doesn't exist. */
	FILE *checkFile;

	checkFile = fopen(fName, "r");

	if (checkFile)
	{
		return 1; //file exists
	}
	else
	{
		return 0; //file doesn't exist
	}
}

int dirExists (char * fName)
{
	/* returns 1 if the file exists an 0 if it doesn't exist. */
	DIR *checkDir;

	checkDir = opendir(fName);

	if (checkDir)
	{
		return 1; //file exists
	}
	else
	{
		return 0; //file doesn't exist
	}
}

int copyFile (char *source, char *dest)
{
	size_t len = 0 ;
    char buffer[BUFSIZ] = { '\0' } ;

    FILE* in = fopen( source, "rb" ) ;
    FILE* out = fopen( dest, "wb" ) ;

    if( in == NULL || out == NULL )
    {
        perror( "An error occured while opening files!!!" ) ;
        in = out = 0 ;
    }
    else    // add this else clause
    {
        while( (len = fread( buffer, 1, BUFSIZ, in)) > 0 )
        {
            fwrite( buffer, 1, len, out ) ;
        }
    
        fclose(in) ;
        fclose(out) ;
		return 0 ;
	}
}

int setPerms(char * fname)
{
    char mode[4]="511";
    char buf[255];

	strcpy(buf,fname);

    int i;
    i = atoi(mode);
    if (chmod (buf,i) < 0)
	{
        printf("error in chmod\n");
		return 0;
	}
	else
	{
		return 1;
	}
}

int CheckUSB()
{
	if (dirExists(USBPS3RoarDir))
		{
			printf("Directory exists!\n");
		}
	else
		{
			mkdir(USBPS3RoarDir);
		
		}

	if (dirExists(USBROMSdir))
		{
			printf("Directory exists!\n");
			CopyUSB(USBROMSdir,ROMSdir);
		}
	else
		{
			printf("Directory DOES NOT exist!\n");
			mkdir(USBROMSdir);
		}

	if (dirExists(USBCASdir))
		{
			printf("Directory exists!\n");
			CopyUSB(USBCASdir,CASdir);
		}
	else
		{
			printf("Directory DOES NOT exist!\n");
			mkdir(USBCASdir);
			
		}

	if (dirExists(USBDSKdir))
		{
			printf("Directory exists!\n");
			CopyUSB(USBDSKdir,DSKdir);
		}
	else
		{
			printf("Directory DOES NOT exist!\n");
			mkdir(USBDSKdir);
			
		}

	if (dirExists(USBSNAPdir))
		{
			printf("Directory exists!\n");
			CopyUSB(USBSNAPdir,SNAPdir);
		}
	else
		{
			printf("Directory DOES NOT exist!\n");
			mkdir(USBSNAPdir);
			
		}

	if (dirExists(USBIMAGEdir)) //ehh, why not. Let people re-skin it. ;-)
		{
			printf("Directory exists!\n");
			CopyUSB(USBIMAGEdir,IMAGEdir);
		}
	else
		{
			printf("Directory DOES NOT exist!\n");
			mkdir(USBIMAGEdir);
			
		}

	if (dirExists(USBSOUNDdir)) //ehh, why not. Let people re-skin it. ;-)
		{
			printf("Directory exists!\n");
			CopyUSB(USBSOUNDdir,SOUNDdir);

		}
	else
		{
			printf("Directory DOES NOT exist!\n");
			mkdir(USBSOUNDdir);
			
		}

	if (dirExists(USBBIOSdir))
		{
			printf("Directory exists!\n");
			CopyUSB(USBBIOSdir,BIOSdir);
		}
	else
		{
			printf("Directory DOES NOT exist!\n");
			mkdir(USBBIOSdir);
			
		}
}

int CopyUSB(char * srcPath, char *dstPath)
{
	int i,x=0;
	
	char *tmp, *srcFileName, *dstFileName;

	srcFileName = malloc(256);
	memset(srcFileName, 0x00, 256);

	dstFileName = malloc(256);
	memset(dstFileName, 0x00, 256);

	int dirs = 0;
	char *fileNames[5000]; //warning 5000 is arbitrary
	struct dirent *dp;

	DIR *exists;
	

    /* Allocate memory for the filenames */
	for ( i = 0; i <= 5000; i++)
	    {
			fileNames[i] = malloc(2560);
			memset(fileNames[i], 0x00, 2560);
        }

	DIR *dir = opendir(srcPath);
	while ((dp=readdir(dir)) != NULL) {
		tmp = path_cat("", dp->d_name);
		strcpy(fileNames[x], tmp);
		free(tmp);
		tmp=NULL;
		if (strcmp(fileNames[x],".") & strcmp(fileNames[x],".."))
		{
			strcpy(srcFileName, srcPath);
			strcat(srcFileName, fileNames[x]);
			strcpy(dstFileName, dstPath);
			strcat(dstFileName, fileNames[x]);
			x++;

			copyFile (srcFileName, dstFileName);
			setPerms(dstFileName);
		}
	}

}

void displayStatic(int Egg)
{
	int loops;

	static int currentStatic;
	
	SDL_Surface *CRT;
	tvScreen.x = 136; //location on screen
	tvScreen.y = 70; 
	tvScreen.w = 320;
	tvScreen.h = 240;
	CRTRect.x = 100; //location on 'static' surface
	CRTRect.y = 150; //change me
	CRTRect.w = 320;
	CRTRect.h = 240;

	loops = 0;
	// TV Screen is 320, 240
		
	if (Egg)
	{
		scrollerRect.w=320;
		scrollerRect.h=30;
		tickerRect.x=136;
		tickerRect.y=170;
		tickerRect.w=320;
		tickerRect.h=30;
		
		/*scrollerRect.x=0;
		scrollerRect.y=0;
		scrollerRect.w=100;
		scrollerRect.h=10;
		tickerRect.x=136;
		tickerRect.y=70;
		tickerRect.w=100;
		tickerRect.h=10;*/
	}

	while (loops <100){

		CRTRect.x = rand()%400; //size of screen minus size of coco display
		CRTRect.y = rand()%280; //size of screen minus size of coco display

		switch (currentStatic)
		{
			case 1:
				CRT = staticScreen1;
				currentStatic++;
				break;
			case 2:
				CRT = staticScreen2;
				currentStatic++;
				break;
			case 3:
				CRT = staticScreen3;
				currentStatic = 1;
				break;
			default:
				CRT = staticScreen1;
				currentStatic = 2;
				break;		
		}
		SDL_BlitSurface(CRT, &CRTRect, screen, &tvScreen);
		SDL_UpdateRect(screen, tvScreen.x, tvScreen.y, CRTRect.w, CRTRect.h);
		SDL_Delay(50);
		loops ++;
	}
	loops =0;
	CRT = testPattern1;
	while (loops <4800){

		if (Egg)
		{
			SDL_BlitSurface(CRT, 0, screen, &tvScreen);
			SDL_UpdateRect(screen, tvScreen.x, tvScreen.y, CRTRect.w, CRTRect.h);
			addEgg();
		}
	loops++;
	printf("loops = %d\n", loops);
	}
	tvScreen.x = 136; //location on screen
	tvScreen.y = 70; 
	tvScreen.w = 320;
	tvScreen.h = 240;
	CRTRect.x = 100; //location on 'static' surface
	CRTRect.y = 150; //change me
	CRTRect.w = 320;
	CRTRect.h = 240;
	scrollerRect.x = 0;
}

void addEgg()
{

			DrawTTF(0,0,"                                                                       _.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-._--==>   Greetz go out to all the people that helped make this possible-- you know who you are. You know who I am, mentioning names would just be lame and self-serving. It's time for this scene to come together and unite! Back to a simpler time, when the demos were much cooler than this and people did them just because they could, and not necessarily for the credibility. Screw cred, the people who matter know what you've done. Respect will come from them, and that's what matters. Those of you who want CFW 3.whatever-- you'll only respect someone as long as they give you what you want. Not one second more! It's time to stop chasing after the elusive 3.whatever CFW, once there is a FW out that has features we need we'll crack it-- have no doubt. One final thing, a big shout out at PSX-SCENE --- You SUX PSX-SCENE, quit censoring our voices as developers! You've already lost the big boys, because they know the shit you've tried in the past, and the rest are leaving too. Now you are left with just getting the news second hand... You will never be first on our release list again!    <==--_.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-.__.-'^^'-._                                                    ", text_fontMedium, Red, scroller1);
			SDL_UpdateRect(scroller1, 0, 0, 0, 0);
			scrollerRect.x= scrollerRect.x+3;
			printf("scrollerRect.x = %d\n", scrollerRect.x);
			if (scrollerRect.x > 7400)
			{
				scrollerRect.x = 0;
			}

			SDL_BlitSurface(scroller1, &scrollerRect, screen, &tickerRect);
			SDL_UpdateRect(screen, tickerRect.x, tickerRect.y, scrollerRect.w, scrollerRect.h);
			SDL_Delay(30);
			SDL_BlitSurface(backgroundScreen, &scrollerRect, screen, &tickerRect);
			tickerRect.y = tickerRect.y + direction;
			if ( tickerRect.y > 275)
				direction = -3;
			if ( tickerRect.y < 90)
				direction = 3;

}

void redrawBackground()
{
	SDL_BlitSurface(backgroundScreen, 0, screen, 0);
}

char * LoadRom(char * path, char * file)
{
	Populatewindow(path, file);
	strncat(path, file, strlen(file));
	strncpy(file, path, strlen(path));
	redrawBackground();
	return file;
}

void DrawTTF(int x,int y,char * text, TTF_Font *theFont, SDL_Color Color, SDL_Surface *screen)
{
	SDL_Surface *sText = TTF_RenderText_Solid( theFont, text, Color );
	SDL_Rect rcDest = {x,y,0,0};
	SDL_BlitSurface( sText,NULL, screen,&rcDest );
	SDL_FreeSurface( sText );
}

void ps3InitDisplay()
{
	/* Loads all the bitmaps into SDL Surfaces for later use */
	char *static1fname;
	static1fname = malloc(256);
	memset(static1fname, 0x00, 256);
	
	char *static2fname;
	static2fname = malloc(256);
	memset(static2fname, 0x00, 256);
	
	char *static3fname;
	static3fname = malloc(256);
	memset(static3fname, 0x00, 256);
	
	char *scroller1fname;
	scroller1fname = malloc(256);
	memset(scroller1fname, 0x00, 256);

	char *testPattern1fname;
	testPattern1fname = malloc(256);
	memset(testPattern1fname, 0x00, 256);

	char *openDialogfname;
	openDialogfname = malloc(256);
	memset(openDialogfname, 0x00, 256);

	char *saveDialogfname;
	saveDialogfname = malloc(256);
	memset(saveDialogfname, 0x00, 256);

	strcpy(static1fname,IMAGEdir);
	strcpy(static2fname,IMAGEdir);
	strcpy(static3fname,IMAGEdir);
	strcpy(scroller1fname,IMAGEdir);
	strcpy(testPattern1fname,IMAGEdir);
	strcpy(openDialogfname,IMAGEdir);
	strcpy(saveDialogfname,IMAGEdir);

	strncat(static1fname,static1,strlen(static1));
	strncat(static2fname,static2,strlen(static2));
	strncat(static3fname,static3,strlen(static3));
	strncat(scroller1fname,strscroller1,strlen(strscroller1));
	strncat(testPattern1fname,strtestPattern1,strlen(strtestPattern1));
	strncat(openDialogfname,strOpenDialog,strlen(strOpenDialog));
	strncat(saveDialogfname,strSaveDialog,strlen(strSaveDialog));

	staticScreen1 = IMG_Load(static1fname);
	staticScreen2 = IMG_Load(static2fname);
	staticScreen3 = IMG_Load(static3fname);
	scroller1 = IMG_Load(scroller1fname);
	testPattern1 = IMG_Load(testPattern1fname);
	openDialog = IMG_Load(openDialogfname);
	saveDialog = IMG_Load(saveDialogfname);

}
char inKeys()
{
	SDL_Event keyevent;    //The SDL event that we will poll to get events.
	bool upup;             //Whether the up key is up.
	bool downup;           //Likewise for the down key.
	bool leftup;           //You get the drift.
	bool rightup; 
 
	upup = true;           //Initialize all the key booleans to true.
	downup = true;
	leftup = true;
	rightup = true;

	int charx, chary;
	int charyvel;
	int charxvel;

	char keyPressed;
 
	while (SDL_PollEvent(&keyevent))   //Poll our SDL key event for any keystrokes.
	{
	if (keyevent.type == SDL_KEYDOWN)
	{
	    switch(keyevent.key.keysym.sym){
		  case SDLK_LEFT:
	        leftup = false;
		    charxvel = -1;
			break;
	      case SDLK_RIGHT:
		    rightup = false;
	        charxvel = 1;
			break;
	      case SDLK_UP:
		    upup = false;
			charyvel = -1;
	        break;
		  case SDLK_DOWN:
			downup = false;
			charyvel = 1;
	        break;
		  default:
			keyPressed = keyevent.key.keysym.unicode;
			break;
	    }
	}
	if (keyevent.type == SDL_KEYUP)
	{
	    switch(keyevent.key.keysym.sym){
		case SDLK_LEFT:
	        leftup = true;
		    charxvel = 0;
			break;
	      case SDLK_RIGHT:
		    rightup = true;
			charxvel = 0;
	        break;
		  case SDLK_UP:
			upup = true;
			charyvel = 0;
	        break;
		case SDLK_DOWN:
	        downup = true;
			charyvel = 0;
	        break;
		default:
			keyPressed = 0;
		    break;
	    }
	  }
	}
	if (!upup) charyvel -= 1;  //These continue to accelerate the character if the user is holding down a button.
	if (!downup) charyvel += 1; 
	if (!rightup) charxvel += 1;
	if (!leftup) charxvel -= 1;
	 
	charx += charxvel;
	chary += charyvel;

	return keyPressed;
}
void CreateFile (char *dir_path, char * selectedFile, char * fType)
{
	int i, loop = 0;
	char *newFile;
	char *tmpptr;
	char *temp;
	int currentChar = 1;
	char inKey;
	int Len=0;

	newFile = malloc(256);
	memset(newFile, 0x00, 256);
	*newFile = "newSnap";

	temp = malloc(256);
	memset(temp, 0x00, 256);
	*temp = "newSnap";

	SDL_Rect rect = {0,0,427,233};
	SDL_BlitSurface(saveDialog, &rect, screen, &rect);
	SDL_UpdateRect(screen, 0, 0, rect.w, rect.h);

	do
	{
		inKey = inKeys();
		if (inKey != 0)
		{
			SDL_BlitSurface(saveDialog, &rect, screen, &rect);
			SDL_UpdateRect(screen, 0, 0, rect.w, rect.h);
			Len = strlen(newFile)-1;

			switch (inKey)
				{
				case 0x8:
					//backspace
					if (Len >= 0)
					{
						strncpy(temp, newFile, Len);
						temp[Len] = '\0';
						strcpy(newFile,temp);
					}
					break;
				case 0x0d:
					//enter
					loop = 1;
					break;
				default:
					if (Len < 25) //yes, 24 characters is arbitrary, but it fits in the window if you use all W's. ;-P
					{
						sprintf(newFile, "%s%c%c",newFile ,inKey, '\0');
					}
					break;
				}
		}
	/*copy directory into file and selected file into file*/
	DrawTTF(78,103,newFile, text_fontMedium, Blue, screen);
	SDL_UpdateRect(screen, 0, 0, 427, 233);
	
	} while (loop == 0);

	strncpy(selectedFile,dir_path, strlen(dir_path)); //start with the directory to save the file
	strncpy(selectedFile + strlen(selectedFile),newFile, strlen(newFile)); //add the filename given, adding to the existing string
	strncpy(selectedFile + strlen(selectedFile),fType, strlen(fType)); //add the file extention provided
	redrawBackground();

}
void Populatewindow(char *dir_path, char * selectedFile)
{
	/*Init PS3 Pads and such*/
	int i, x=0;
	int loop=0;
	int numberFiles=34;
	int fileStart=0, fileEnd=29, displayFiles=29;
	int highlight = 0;
	int realSelected = 0;
	struct dirent *dp;

	char *fileNames[5000]; //warning 5000 is arbitrary

    /* Allocate memory for the filenames */
	for ( i = 0; i <= 5000; i++)
	    {
			fileNames[i] = malloc(2560);
			memset(fileNames[i], 0x00, 2560);
        }

	/* enter existing path to directory below */
	DIR *dir = opendir(dir_path);
	while ((dp=readdir(dir)) != NULL) {
		char *tmp;
		tmp = path_cat("", dp->d_name);
		strcpy(fileNames[x], tmp);
		free(tmp);
		tmp=NULL;
		if (strcmp(fileNames[x],".") & strcmp(fileNames[x],".."))
		{
			x++;
		}
	}
	closedir(dir);
	numberFiles = x;
	if ((fileEnd > numberFiles) & (displayFiles > numberFiles))
	{
		fileEnd = numberFiles;
		displayFiles = numberFiles;
	}

	SDL_Rect rect = {0,0,486,425};
	SDL_BlitSurface(openDialog, &rect, screen, &rect);
	SDL_UpdateRect(screen, 0, 0, rect.w, rect.h);
	do
	{
		padInfo padinfo;
		padData paddata;	
		ioPadInit(7);
		ioPadGetInfo ( &padinfo ) ; // Check the pads
		
		/*Check Pads*/
		for ( i = 0 ; i < MAX_PADS ; i++ ) 
		{
			if ( padinfo.status[i] ) 
			{
				ioPadGetData ( i, &paddata ) ;
				if ( paddata.BTN_START ) 
				{
					/* reset button status */
					paddata.BTN_START = 0; 
				}
				else if ( paddata.BTN_SELECT ) 
				{
					/* reset button status */
					paddata.BTN_SELECT = 0; 
				}
				else if ( paddata.BTN_CROSS ) 
				{
					strncpy(selectedFile,fileNames[realSelected], strlen(fileNames[realSelected]));
					loop=1;
					/* reset button status */
					paddata.BTN_CROSS = 0; 
				}
				else if ( paddata.BTN_CIRCLE ) 
				{
					/* reset button status */
					paddata.BTN_CIRCLE = 0; 
				}
				else if ( paddata.BTN_SQUARE ) 
				{
					/* reset button status */
					paddata.BTN_SQUARE = 0; 
				}
				else if ( paddata.BTN_TRIANGLE ) 
				{
					/* reset button status */
					paddata.BTN_TRIANGLE = 0; 
				}
				else if ( paddata.BTN_UP ) 
				{	
					SDL_BlitSurface(openDialog, &rect, screen, &rect);
					//SDL_UpdateRect(screen, 0, 0, rect.w, rect.h);
					highlight--;
					if (realSelected > 0)
						realSelected--;
					if (highlight < 0)
					{
						fileStart--;
						highlight = 0;
					}
					if (fileStart < 0)
						fileStart = 0;
					fileEnd = fileStart + displayFiles;
					SDL_Delay(125);
				}
				else if ( paddata.BTN_DOWN ) 
				{
					SDL_BlitSurface(openDialog, &rect, screen, &rect);
					if (realSelected < numberFiles-1)
						realSelected++;
					if (highlight < displayFiles)
					{
						highlight++;
					}
					if (highlight > (displayFiles-1))
					{
						highlight = (displayFiles-1);
						fileStart++;
					}
					if (fileStart  > (numberFiles -displayFiles))
					{
						fileStart = numberFiles-displayFiles;
					}
					fileEnd = fileStart + displayFiles;
					SDL_Delay(125);
				}
			ioPadClearBuf (i);
			}
		}

	/* Display the files in the scrolling window */
	for (x=0;x<displayFiles;x++)
	{
		if ((fileStart + x) <= (fileStart + sizeof(fileNames)))
		{
			if (x == highlight)
			{
				SDL_Rect selecter = {87,75+(x*10),373,13};
				SDL_FillRect(screen, &selecter, SDL_MapRGB(screen->format,0xbc,0xd3, 0xeb));/* Interface Background */
			}
			DrawTTF(90,75+(x*10),fileNames[fileStart + x], text_fontSmall, Blue, screen);
		}
	}
	SDL_UpdateRect(screen, 0, 0, 486, 425);

	} while (loop == 0);
}

int listDir (const char *dir_path, char *fileNames[]) {
	struct dirent *dp;
	int x = 0;

    /* enter existing path to directory below */
	DIR *dir = opendir(dir_path);
	while ((dp=readdir(dir)) != NULL) {
		char *tmp;
		tmp = path_cat(dir_path, dp->d_name);
		fileNames[x]= tmp;
		free(tmp);
		tmp=NULL;
		x++;
	}
	closedir(dir);
	return x;
}

char *path_cat (const char *str1, char *str2)
{
	size_t str1_len = strlen(str1);
	size_t str2_len = strlen(str2);
	char *result;
	result = malloc((str1_len+str2_len+1)*sizeof *result);
	strcpy (result,str1);
	int i,j;
	for(i=str1_len, j=0; ((i<(str1_len+str2_len)) && (j<str2_len));i++, j++) {
		result[i]=str2[j];
	}
	result[str1_len+str2_len]='\0';
	return result;
}
