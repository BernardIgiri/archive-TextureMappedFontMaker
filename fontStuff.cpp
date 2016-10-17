#include "fontStuff.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>

void CCharecter::Print(int &x,int y,const CTypeFace *pTypeFace)
{ // printed down from top left
	glBegin(GL_QUADS);
		glTexCoord2f(m_left,m_bottom);
		glVertex2i(x,y-pTypeFace->m_typeHeight);
		glTexCoord2f(m_right,m_bottom);
		glVertex2i(x+m_width,y-pTypeFace->m_typeHeight);
		glTexCoord2f(m_right,m_top);
		glVertex2i(x+m_width,y);
		glTexCoord2f(m_left,m_top);
		glVertex2i(x,y);
	glEnd();
	x+=m_width+pTypeFace->m_charSpacing;
}

void CCharecter::SeqPrint(int &x,int y,const CTypeFace *pTypeFace)
{ // printed down from top left
	glTexCoord2f(m_left,m_bottom);
	glVertex2i(x,y-pTypeFace->m_typeHeight);
	glTexCoord2f(m_right,m_bottom);
	glVertex2i(x+m_width,y-pTypeFace->m_typeHeight);
	glTexCoord2f(m_right,m_top);
	glVertex2i(x+m_width,y);
	glTexCoord2f(m_left,m_top);
	glVertex2i(x,y);
	x+=m_width+pTypeFace->m_charSpacing;
}


void CTypeFace::Print(int x,int y,const char *string)
{
	if (!string)
		return; // reject null strings
	if (!strlen(string))
		return; // reject empty strings
	glBegin(GL_QUADS);
		while (*string)
		{
			if (*string>31&&*string<128)
				m_chars[*string-32].SeqPrint(x,y,this);
			else // use char 95 for invalid chars
				m_chars[95].SeqPrint(x,y,this);
			string++;
		}
	glEnd();
}

void CTypeFace::PrintF(int x,int y,const char *string,bool isMultiLine)
{
	if (!string)
		return; // reject null strings
	if (!strlen(string))
		return; // reject empty strings
	int xMargin = x; // copy x for carriage returns.
	glBegin(GL_QUADS);
		while (*string)
		{
			if (*string>31&&*string<128)
				m_chars[*string-32].SeqPrint(x,y,this);
			else
			{
				switch (*string)
				{
				case '\n': // newline char
				case '\f': // form feed char
				case '\r': // carraige return char
					if (isMultiLine)
					{
						y-=m_typeHeight; // note y axis is inverted in opengl
						x=xMargin;
					}
					else
						m_chars[95].SeqPrint(x,y,this); // use char 95 for invalid chars
					break;
				case '\t': // tab char
					x+=m_chars[0].m_width*8; // width of tab is 8 spaces
					break;
				default:
					m_chars[95].SeqPrint(x,y,this); // use char 95 for invalid chars
					break;
				}
			}
			string++;
		}
	glEnd();
}

void CTypeFace::CalcPrintArea(int &width,int &height,const char *string,bool isMultiLine)
{
	if (!string)
		return; // reject null strings
	if (!strlen(string))
		return; // reject empty strings
	width=0;
	int curLineWidth=0;
	height=m_typeHeight;
	while (*string)
	{
		if (*string>31&&*string<128)
			curLineWidth+=m_chars[*string-32].m_width;
		else
		{
			switch (*string)
			{
			case '\n': // newline char
			case '\f': // form feed char
			case '\r': // carraige return char
				if (isMultiLine)
				{
					if (curLineWidth>width)
						width=curLineWidth; // get width of widest line
					height+=m_typeHeight; // increase height for every new line
					curLineWidth= -m_charSpacing; // to nullify addition of m_charSpacing
				}
				else
					curLineWidth+=m_chars[95].m_width; // use char 95 for invalid chars
				break;
			case '\t': // tab char
				curLineWidth+=m_chars[0].m_width*8; // width of tab is 8 spaces
				break;
			default:
				curLineWidth+=m_chars[95].m_width; // use char 95 for invalid chars
				break;
			}
		}
		curLineWidth+=m_charSpacing;
		string++;
	}
	if (curLineWidth>width)
		width=curLineWidth; // get width of widest line
}

void CTypeFace::EstPrintArea(int &width,int &height,int rows,int cols)
{
	width = cols*(m_typeWidth+m_charSpacing);
	height = rows*m_typeHeight;
}

void CTypeFace::EstCharSpace(int &rows,int &cols,int width,int height)
{
	cols = width/(m_typeWidth+m_charSpacing);
	rows = height/m_typeHeight;
}

void CTypeFace::CalcCharCoords(int &left,int &top,int &right,int &bottom,int index,const char *string,bool isMultiLine)
{
	if (!string)
		return; // reject null strings
	if (!strlen(string))
		return; // reject empty strings
	top=left=right=0;
	bottom= -m_typeHeight;

	int i=0;	
	while ((*string!=NULL)&&(i<=index))
	{
		left=right;
		if (*string>31&&*string<128)
			right+=m_chars[*string-32].m_width;
		else
		{
			switch (*string)
			{
			case '\n': // newline char
			case '\f': // form feed char
			case '\r': // carraige return char
				if (isMultiLine)
				{
					if (i==index)
					{
						right+=(-m_charSpacing);
						left=right;
						right++;
						return;
					}
					top=bottom;
					bottom-=m_typeHeight; // note y axis is inverted in opengl
					left=0;
					right=-m_charSpacing;
				}
				else
					right+=m_chars[95].m_width; // use char 95 for invalid chars
				break;
			case '\t': // tab char
				right+=m_chars[0].m_width*8-m_charSpacing;; // width of tab is 8 spaces
				break;
			default:
				right+=m_chars[95].m_width; // use char 95 for invalid chars
				break;
			}
		}
		right+=m_charSpacing;
		string++;
		i++;
	}
	right-=m_charSpacing;
}

int CTypeFace::GetSelectedChar(int cX,int cY,const char *string,bool isMultiLine)
{
	if (!string)
		return -1; // reject null strings
	if (!strlen(string))
		return -1; // reject empty strings
	int top=0,left=0,right=0,
		bottom = -m_typeHeight;

	int i=0;	
	while (*string)
	{
		left=right;
		if (*string>31&&*string<128)
			right+=m_chars[*string-32].m_width;
		else
		{
			switch (*string)
			{
			case '\n': // newline char
			case '\f': // form feed char
			case '\r': // carraige return char
				if (isMultiLine)
				{
					top=bottom;
					bottom-=m_typeHeight; // note y axis is inverted in opengl
					left=0;
					right=-m_charSpacing;
				}
				else
					right+=m_chars[95].m_width; // use char 95 for invalid chars
				break;
			case '\t': // tab char
				right+=m_chars[0].m_width*8-m_charSpacing;; // width of tab is 8 spaces
				break;
			default:
				right+=m_chars[95].m_width; // use char 95 for invalid chars
				break;
			}
		}
		right+=m_charSpacing;
		if ((cX>=left&&cX<right)&&
			(cY>=bottom&&cY<=top))
			return i;
		string++;
		i++;
	}
	return -1;
}
