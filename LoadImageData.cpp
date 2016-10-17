#include "LoadImageData.h"
#include <il/ilu.h>
#include <malloc.h>
#include <memory.h>

unsigned char *LoadImageData(char *filename, int *width, int *height, int *bpp, size_t *size)
{
	static bool isIntialized=false;
	if (!isIntialized)
	{
		ilInit();
		iluInit();
		isIntialized=true;
	}
	if (ilLoadImage(filename)==IL_FALSE)
		return NULL;
	ILubyte *data = NULL;
	data = ilGetData();
	if (data==NULL) // get data failed
		return NULL;
	ILinfo imageInfo;
	iluGetImageInfo(&imageInfo);
	if (imageInfo.Data==NULL)
		return NULL;
	(*width)=(int)imageInfo.Width;
	(*height)=(int)imageInfo.Height;
	(*bpp)=(int)imageInfo.Bpp*8;
	(*size)=(size_t)imageInfo.SizeOfData;
	unsigned char *dataCopy = (unsigned char *)malloc(imageInfo.SizeOfData);
	if (dataCopy!=NULL)
		memcpy(dataCopy,imageInfo.Data,imageInfo.SizeOfData);
			

	return dataCopy;
}

CBMI::CBMI()
{
	memset(this,NULL,sizeof(this));
}

CBMI::~CBMI()
{
	if (m_data)
		free(m_data);
}