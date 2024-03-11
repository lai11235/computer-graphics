///////////////////////////////////////////////////////////////////////////////
//
//      TargaImage.h                            Author:     Stephen Chenney
//                                              Modified:   Eric McDaniel
//                                              Date:       Fall 2004
//
//      Class to manipulate targa images.  You must implement the image 
//  modification functions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _TARGA_IMAGE_H_
#define _TARGA_IMAGE_H_

#include <Fl/Fl.h>
#include <Fl/Fl_Widget.h>
#include <stdio.h>
#include <utility>

class Stroke;
class DistanceImage;

class TargaImage
{
    // methods
    public:
	    TargaImage(void);
            TargaImage(int w, int h);
	    TargaImage(int w, int h, unsigned char *d);
            TargaImage(const TargaImage& image);
	    ~TargaImage(void);

        unsigned char*	To_RGB(void);	            // Convert the image to RGB format,
        bool Save_Image(const char*);               // save the image to a file
        static TargaImage* Load_Image(char*);       // Load a file and return a pointer to a new TargaImage object.  Returns NULL on failure

        bool To_Grayscale();

        bool Quant_Uniform();
        bool Quant_Populosity();
        bool Quant_Median();

        bool Dither_Threshold();
        bool Dither_Random();
        bool Dither_FS();
        bool Dither_FS_Swatches(float* swatch, int type);
        bool Dither_Bright();
        bool Dither_Cluster();
        bool Dither_Color();

        bool Comp_Over(TargaImage* pImage);
        bool Comp_In(TargaImage* pImage);
        bool Comp_Out(TargaImage* pImage);
        bool Comp_Atop(TargaImage* pImage);
        bool Comp_Xor(TargaImage* pImage);

        bool Difference(TargaImage* pImage);

        bool Filter_Box();
        bool Filter_Bartlett();
        bool Filter_Bartlett_NM(int type, int n, int m, float base);
        bool Filter_Gaussian();
        bool Filter_Gaussian_N(unsigned int N);
        bool Filter_Edge();
        bool Filter_Enhance();

        bool NPR_Paint();

        bool Half_Size();
        bool Double_Size();
        bool Resize(float scale);
        bool Rotate(float angleDegrees);

        float cluster_matrix[4][4] =
        { 
            {0.7059, 0.3529, 0.5882, 0.2353},
            {0.0588, 0.9412, 0.8235, 0.4118},
            {0.4706, 0.7647, 0.8824, 0.1176},
            {0.1765, 0.5294, 0.2941, 0.6471} 
        };

        //Stroing with the formation (x, y)
        int dither_fs_move_odd[4][2] =
        {
            {-1, 0},
            {1, 1},
            {0, 1},
            {-1, 1}
        };

        //Stroing with the formation (x, y)
        int dither_fs_move_even[4][2] =
        {
            {1, 0},
            {1, 1},
            {0, 1},
            {-1, 1}
        };

        float dither_fs_error_rate[4] =
        { 
            0.4375f , 0.0625f, 0.3125f, 0.1875f
        };

        int dither_color_red[8] =
        {
            0, 36, 73, 109, 146, 182, 219, 255
        };

        int dither_color_green[8] =
        {
            0, 36, 73, 109, 146, 182, 219, 255
        };

        int dither_color_blue[4] =
        {
            0, 85, 170, 255
        };

        float  bartlett_filter_matrix[5][5] =
        {
            {1, 2, 3, 2, 1},
            {2, 4, 6, 4, 2},
            {3, 6, 9, 6, 3},
            {2, 4, 6, 4, 2},
            {1, 2, 3, 2, 1}
        };

        float gaussian_filter_matrix[5][5] = 
        {
            {1,  4,  6,  4, 1},
            {4, 16, 24, 16, 4},
            {6, 24, 36, 24, 6},
            {4, 16, 24, 16, 4},
            {1,  4,  6,  4, 1}
        };

        float half_filter_matrix[3][3] =
        {
            {1, 2, 1},
            {2, 4, 2},
            {1, 2, 1}
        };

        float double_filter_matrix1[3][3] =
        {
            {1, 2, 1},
            {2, 4, 2},
            {1, 2, 1}
        };

        float double_filter_matrix2[4][4] =
        {
            {1, 3, 3, 1},
            {3, 9, 9, 3},
            {3, 9, 9, 3},
            {1, 3, 3, 1}
        };

        float double_filter_matrix3[4][3] =
        {
            {1, 2, 1},
            {3, 6, 3},
            {3, 6, 3},
            {1, 2, 1}
        };

    private:
	// helper function for format conversion
        void RGBA_To_RGB(unsigned char *rgba, unsigned char *rgb);

        // reverse the rows of the image, some targas are stored bottom to top
	TargaImage* Reverse_Rows(void);

	// clear image to all black
        void ClearToBlack();

	// Draws a filled circle according to the stroke data
        void Paint_Stroke(const Stroke& s);
    
    // Determine if the postion is valid on image
        bool Is_Valid_Img_Pos(int x, int y);
    
    // Determine if the position is in the range
        int Mask_Pos_In_Range(int x, int y);

    // Find the closest palette in dither color algorithm
        int Find_Proper_Dither_Color(int type, int val);

    // Calculate the average value of color in 5 * 5 area
        int Box_Filter_Fetch_Value(int x, int y, int *swatches);

    // Calculate the value of color in 5 * 5 area with specific rate(Bartlett Filter)
        int Bartlett_Filter_Fetch_Value(int x, int y, int* swatches);

        int Bartlett_Filter_NM_Fetch_Value(int x, int y, int* swatches, int n, int m, int type, float bases);

   // Calculate the value of color in 5 * 5 area with specific rate(Gaussian Filter)
        int Gaussian_Filter_Fetch_Value(int x, int y, int* swatches);

        float Find_Matrix_Val_With_Type(int x, int y, int type)
        {
            switch (type)
            {
            case 0:
                return bartlett_filter_matrix[y][x];
            case 1:
                return half_filter_matrix[y][x];
            case 2:
                return double_filter_matrix1[y][x];
            case 3:
                return double_filter_matrix2[y][x];
            case 4:
                return double_filter_matrix3[y][x];
            }
        }
    // members
    public:
        int		width;	            // width of the image in pixels
        int		height;             // height of the image in pixels
        int     data_array_size;    // size of the data array
        int     img_size;           // size of the image
        unsigned char	*data;	    // pixel data for the image, assumed to be in pre-multiplied RGBA format.
        int DARK = 0;
        int BRIGHT = 255;

};

class Stroke { // Data structure for holding painterly strokes.
public:
   Stroke(void);
   Stroke(unsigned int radius, unsigned int x, unsigned int y,
          unsigned char r, unsigned char g, unsigned char b, unsigned char a);
   
   // data
   unsigned int radius, x, y;	// Location for the stroke
   unsigned char r, g, b, a;	// Color
};

class Color {
public:
    int red;
    int green;
    int blue;
};


#endif


