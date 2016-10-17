#ifndef __INCLUDED_LOADIMAGEDATA__
#define __INCLUDED_LOADIMAGEDATA__

unsigned char *LoadImageData(char *filename, int *width, int *height, int *bpp, size_t *size);

class CBMI
{
public:
	CBMI();
	~CBMI();
	unsigned char*	m_data; // pixel data
	int				m_width; // image width
	int				m_height; // image height
	int				m_bpp; // bits per pixel
	unsigned		m_size; // size of pixel data
};

#endif//__INCLUDED_LOADIMAGEDATA__