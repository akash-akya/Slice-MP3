#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

const int BitRateIndex[][6] = {
  {0,   0,   0,   0,   0},
  {32,  32,  32,  32,  8},
  {64,  48,  40,  48,  16},
  {96,  56,  48,  56,  24},
  {128, 64,  56,  64,  32},
  {160, 80,  64,  80,  40},
  {192, 96,  80,  96,  48},
  {224, 112, 96,  112, 56},
  {256, 128, 112, 128, 64},
  {288, 160, 128, 144, 80},
  {320, 192, 160, 160, 96},
  {352, 224, 192, 176, 112},
  {384, 256, 224, 192, 128},
  {416, 320, 256, 224, 144},
  {448, 384, 320, 256, 160},
  {-1,  -1,   -1,  -1,  -1}
};

const int SamplesPerFrame[][3] = {
  {384,   384,   384,},
  {1152,  1152,  1152,},
  {1152,  576,   576}
};

const int SamplingRate [] = {
  44100,    22050,    11025,
  48000,    24000,    12000,
  32000,    16000,    8000
};

typedef struct HeadStruct
{
  int mpegVersion;
  int layer;
  int crcProtection;
  int bitRate;
  int samplingRate;
  int padding;
  int privateBit;
  int channel;
  int modeExt;
  int copyRight;
  int original;
  int emphasis;
} HeaderStruct;

int getVal (char buf[], int start, int len)
{
  int i;
  int val = 0;
    
  for(i=start; i<(start+len); i++)
    {
      val <<= 1;
      val |= buf [i];
    }

  return val; 
}

int getVersion (char buf[])
{
  switch (getVal(buf, 11, 2))
    {
    case 0 : return 3;
    case 2 : return 2;
    case 3 : return 1;
    default : return -1;
    }
} 

int getLayer (char buf[])
{
  switch (getVal(buf, 13,2))
    {
    case 0 : return 0;
    case 1 : return 3;
    case 2 : return 2;
    case 3 : return 1;
    default : return -1; 
    }
}

int getBitrate (char buf[])
{
  int row = getVal(buf, 16, 4);
  int col = 0;

  if (getVersion(buf) == 1)
    {
      col = getLayer(buf)-1;
      // return BitRateIndex[row][col];
    }
  else if (getVersion(buf) == 2 || getVersion(buf) == 3)
    {
      col = 3;
      
      if (getLayer(buf)==2 || getLayer(buf) == 3)
	{
	  col++;
	}
    }
  else
    {
      cout << "Bitrate Error !" << endl;
      return 0;
    }
  
  return BitRateIndex[row][col];
}


HeaderStruct parseHeader (unsigned char *t)
{
    int i, j;
    char buf [32];
    HeaderStruct head;
    int h;

    h = *((int *)t);

    for (j=3; j>=0; j--)
    {
      for (i=0; i<8; i++)
	{
	  buf[31-(j*8)-i] = (h & 0x1);
	  h = h >> 1;
	}
    }

    // cout << endl;
    // for (i=0; i<4; i++)
    //   {
    // 	for (j=0; j<8; j++)
    // 	  cout << (int)buf[i*8+j]  << " ";
    // 	cout << endl;
    //   }
    // cout << endl;
    
    head.mpegVersion = getVersion (buf);
    head.layer = getLayer (buf);
    head.crcProtection= getVal(buf, 15, 1);
    head.bitRate      = getBitrate(buf);
    head.samplingRate = SamplingRate[getVal(buf, 20, 2)*2 + getVersion(buf) -1];
    head.padding      = getVal (buf, 22, 1);  
    head.privateBit   = getVal (buf, 23, 1);  
    head.channel      = getVal (buf, 24, 2);
    head.modeExt      = getVal (buf, 26, 2);
    head.copyRight    = getVal (buf, 27, 1);
    head.original     = getVal (buf, 28, 1);
    head.emphasis     = getVal (buf, 29, 2);

    return head;
}

void displayInfo(char name[], HeaderStruct &head, float size, int min, int sec)
{
  	  
  cout << "\nFile         : " << name           << endl;
  cout << "Version      : " << head.mpegVersion  << endl;
  cout << "Layer        : " << head.layer        << endl;
  cout << "CRC          : " << (head.crcProtection ? "False" : "True")<< endl;
  cout << "Bitrate      : " << head.bitRate      << " kbps "<< endl;
  cout << "SmaplingRate : " << head.samplingRate << " Hz "<< endl;
  cout << "Padding      : " << (head.padding ? "Padded" : "Not Padded")      << endl;
  cout << "Private Bit  : " << head.privateBit   << endl;
  cout << "Channel      : " << head.channel      << endl;
  cout << "Mode Ext     : " << head.modeExt      << endl;
  cout << "Padding      : " << (head.copyRight ? "Copyright protected" : "Not copyright protected")      << endl;
  cout << "Original     : " << (head.original ? "Original Media" : "Not original Media")     << endl;
  cout << "Emphasis     : " << head.emphasis      << endl; 
  cout << "File Size    : " << size << " MB" << endl;
  cout << "Duration     : " << min << " : " << sec << endl;

}

int main (int argc, char *argv[])
{
  FILE *mp3File = NULL;
  unsigned char temp[2] = {0,0};
  int count = 0;
  int c=0, p=0, n=0;
  int spf, size;
  int startPos, endPos;
  long frameCount = 0;
  int min, sec;
  float duration;
  float avgBitrate = 0;
  HeaderStruct head;
  
  if (argc < 2 )
    {
      cout << "Usage <pgm> filename" << endl;
      exit (1);
    }
  
  if(!(mp3File = fopen (argv[1], "rb")))
    {
      cout << "Can not open file!!" << endl;
      exit (1);
    }

  n = fgetc (mp3File);
  while (n != EOF && (p != 0xFF || ((c&0xE0) != 0xE0)))
    {
      count++;
      p = c;  c = n;
      n = fgetc (mp3File);
    } 

  cout << "ID Tag Size  : " << count-2 << " Bytes" << endl;
      
  startPos = count;
  fseek(mp3File, -3L, SEEK_CUR);
  while (fread (&temp, 4, sizeof(char), mp3File))
    {
      if(temp[0] == 0xFF && (temp[1] & 0xE0) == 0xE0)
	{	  
	  head = parseHeader(temp);
	  avgBitrate = (avgBitrate*frameCount+head.bitRate)/(frameCount+1);
	  frameCount++; 

	  spf = SamplesPerFrame[head.layer-1][head.mpegVersion-1];
	  size = ((spf / 8 * head.bitRate * 1000) / head.samplingRate)
	    + head.padding;
	  size += (head.crcProtection > 0) ? 0 : 2; 
	  size -= 4;
	}
      else
	{
	  // cout << "End of song data\n";0
	  break;
	}
      fseek(mp3File, size, SEEK_CUR);
    }
      
  endPos = ftell(mp3File);
  fseek(mp3File, 0L, SEEK_END);
  size = ftell(mp3File) /1000000.0 ;

  duration  = ((endPos-startPos) / avgBitrate * 8.0 / 1000) ;

  min = (int) duration / 60;
  sec = (duration-min)*60;

  displayInfo (argv[1], head, size, min, sec);  
  fclose(mp3File);

  if (argc == 5)
    {    
 
      FILE *newFile;
      char tag[4096*2];
      long dur;
      long curDur = 0;
      int avgBitrate = 0;
      int frameCount = 0;


      mp3File = fopen (argv[1], "rb");
      if(!(newFile = fopen (argv[2], "wb")))
	{
	  cout << "Invalid file name" << endl;
	  exit(0);
	} 

      min = atoi (argv[3]); 
      sec = atoi (argv[4]);

      fread (&tag, count-2, sizeof(char), mp3File);
      fwrite(&tag, count-2, sizeof(char), newFile);
      startPos = ftell(newFile);
				      
      dur = (sec + min*60);
      while (curDur <= dur)
	{
	  fread (&temp, 4, sizeof(char), mp3File);

	  head = parseHeader(temp);
      
	  avgBitrate = (avgBitrate*frameCount+head.bitRate)/(frameCount+1);
	  frameCount++;
	  
	  spf = SamplesPerFrame[head.layer-1][head.mpegVersion-1];
	  size = ((spf / 8 * head.bitRate * 1000) / head.samplingRate)
	    + head.padding;
	  size += (head.crcProtection > 0) ? 0 : 2;

	  fseek (mp3File, -4L, SEEK_CUR);
	  fread(&tag, size, sizeof(char), mp3File);
	  fwrite (&tag, size, sizeof(char), newFile);


	  curDur = ((ftell(mp3File)-startPos) / avgBitrate * 8.0 / 1000) ;

	}

      endPos = ftell(newFile);
      fseek(newFile, 0L, SEEK_END);
      size = ftell(newFile) / 1000000.0;

      duration  = ((endPos-startPos) / avgBitrate * 8.0 / 1000) ;

      min = (int) duration / 60;
      sec = duration-min*60;

      displayInfo (argv[2], head, size, min, sec);  

      fclose(newFile);
      fclose(mp3File);
    }

  return 0;
}
