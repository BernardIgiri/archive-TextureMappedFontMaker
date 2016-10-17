#ifndef __INCLUDED_FONTSTUFF__
#define __INCLUDED_FONTSTUFF__

class CTypeFace;

class CCharecter
{
public:
	void Print(int &x,int y,const CTypeFace *pTypeFace);
	void SeqPrint(int &x,int y,const CTypeFace *pTypeFace);

	int m_width; // width of char
	float m_left,
		  m_right,
		  m_top,
		  m_bottom;
};

class CTypeFace
{
public:

	void Print(int x,int y,const char *string);
	void PrintF(int x,int y,const char *string,bool isMultiLine=false);
	void CalcPrintArea(int &width,int &height,const char *string,bool isMultiLine=false);
	void EstPrintArea(int &width,int &height,int rows,int cols);
	void EstCharSpace(int &rows,int &cols,int width,int height);
	void CalcCharCoords(int &left,int &top,int &right,int &bottom,int index,const char *string,bool isMultiLine=false);
	int GetSelectedChar(int cX,int cY,const char *string,bool isMultiLine=false);
	// all I need now is a word wrapper

	int m_texWidth; // width of font texture file
	int m_texHeight; // height of font texture file
	int m_width; // width of font bitmap data
	int m_height; // height of font bitmap data
	int m_typeWidth; // width of char bitmap data
	int m_typeHeight; // height of char bitmap data
	int m_lineHeight; // height of line for underlining text
	int m_charSpacing; // space between chars
	CCharecter m_chars[96]; // character data
};

#endif//__INCLUDED_FONTSTUFF__