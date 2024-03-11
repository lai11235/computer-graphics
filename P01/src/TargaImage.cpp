///////////////////////////////////////////////////////////////////////////////
//
//      TargaImage.cpp                          Author:     Stephen Chenney
//                                              Modified:   Eric McDaniel
//                                              Date:       Fall 2004
//
//      Implementation of TargaImage methods.  You must implement the image
//  modification functions.
//
///////////////////////////////////////////////////////////////////////////////

#include "Globals.h"
#include "TargaImage.h"
#include "libtarga.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

// constants
const int           RED             = 0;                // red channel
const int           GREEN           = 1;                // green channel
const int           BLUE            = 2;                // blue channel
const unsigned char BACKGROUND[3]   = { 0, 0, 0 };      // background color


// Computes n choose s, efficiently
double Binomial(int n, int s)
{
    double        res;

    res = 1;
    for (int i = 1 ; i <= s ; i++)
        res = (n - i + 1) * res / i ;

    return res;
}// Binomial


///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage() : width(0), height(0), data(NULL)
{}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h) : width(w), height(h)
{
   data = new unsigned char[width * height * 4];
   ClearToBlack();
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables to values given.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h, unsigned char *d)
{
    int i;

    width = w;
    height = h;
    data_array_size = width * height * 4;
    img_size = data_array_size / 4;
    data = new unsigned char[width * height * 4];

    for (i = 0; i < width * height * 4; i++)
	    data[i] = d[i];
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Copy Constructor.  Initialize member to that of input
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(const TargaImage& image) 
{
   width = image.width;
   height = image.height;
   data_array_size = width * height * 4;
   img_size = data_array_size / 4;
   data = NULL; 
   if (image.data != NULL) {
      data = new unsigned char[width * height * 4];
      memcpy(data, image.data, sizeof(unsigned char) * width * height * 4);
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Destructor.  Free image memory.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::~TargaImage()
{
    if (data)
        delete[] data;
}// ~TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Converts an image to RGB form, and returns the rgb pixel data - 24 
//  bits per pixel. The returned space should be deleted when no longer 
//  required.
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::To_RGB(void)
{
    unsigned char   *rgb = new unsigned char[width * height * 3];
    int		    i, j;

    if (! data)
	    return NULL;

    // Divide out the alpha
    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = i * width * 4;
	    int out_offset = i * width * 3;

	    for (j = 0 ; j < width ; j++)
        {
	        RGBA_To_RGB(data + (in_offset + j*4), rgb + (out_offset + j*3));
	    }
    }

    return rgb;
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Save the image to a targa file. Returns 1 on success, 0 on failure.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Save_Image(const char *filename)
{
    TargaImage	*out_image = Reverse_Rows();

    if (! out_image)
	    return false;

    if (!tga_write_raw(filename, width, height, out_image->data, TGA_TRUECOLOR_32))
    {
	    cout << "TGA Save Error: %s\n", tga_error_string(tga_get_last_error());
	    return false;
    }

    delete out_image;

    return true;
}// Save_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Load a targa image from a file.  Return a new TargaImage object which 
//  must be deleted by caller.  Return NULL on failure.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Load_Image(char *filename)
{
    unsigned char   *temp_data;
    TargaImage	    *temp_image;
    TargaImage	    *result;
    int		        width, height;

    if (!filename)
    {
        cout << "No filename given." << endl;
        return NULL;
    }// if

    temp_data = (unsigned char*)tga_load(filename, &width, &height, TGA_TRUECOLOR_32);
    if (!temp_data)
    {
        cout << "TGA Error: %s\n", tga_error_string(tga_get_last_error());
	    width = height = 0;
	    return NULL;
    }
    temp_image = new TargaImage(width, height, temp_data);
    free(temp_data);

    result = temp_image->Reverse_Rows();

    delete temp_image;

    return result;
}// Load_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Determine the position visited if is a valid region to move
//          return True if the position is in valid position
// 
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Is_Valid_Img_Pos(int x, int y)
{
    return x >= 0 && x < width && y >= 0 && y < height;
}


///////////////////////////////////////////////////////////////////////////////
//
//      Determine the position visited if is a valid region to move:
// 
//          return -1 : x and y are not both in range
//          return 0  : x is not in range
//          return 1  : y is not in range
//          retrun 2  : x and y are both in range
// 
///////////////////////////////////////////////////////////////////////////////
int TargaImage::Mask_Pos_In_Range(int x, int y)
{
    bool x_in = x >= 0 && x < width;
    bool y_in = y >= 0 && y < height;
    if (!x_in && !y_in)
        return -1;
    else if (x_in && !y_in)
        return 0;
    else if (!x_in && y_in)
        return 1;
    else
        return 2;
}


///////////////////////////////////////////////////////////////////////////////
//
//      Finding the most closest plaette in seperate color swatches
//          type 0: red
//          type 1: green
//          type 2: blue
// 
///////////////////////////////////////////////////////////////////////////////
int TargaImage::Find_Proper_Dither_Color(int type, int val)
{
    int i;
    switch (type)
    {
    case 0:// Red
        if (val >= dither_color_red[7])
            return dither_color_red[7];
        if (val <= dither_color_red[0])
            return dither_color_red[0];
        for (i = 0; i < 8; i++)
        {
            if (val < dither_color_red[i])
                break;
            else if (val == dither_color_red[i])
                return dither_color_red[i];
        }
        
        return val - dither_color_red[i - 1] < dither_color_red[i] - val ? dither_color_red[i - 1] : dither_color_red[i];
    case 1:// Green
        if (val >= dither_color_green[7])
            return dither_color_green[7];
        if (val <= dither_color_green[0])
            return dither_color_green[0];
        for (i = 0; i < 8; i++)
        {
            if (val < dither_color_green[i])
                break;
            else if (val == dither_color_green[i])
                return dither_color_green[i];
        }

        return val - dither_color_green[i - 1] < dither_color_green[i] - val ? dither_color_green[i - 1] : dither_color_green[i];
    case 2: //Blue
        if (val >= dither_color_blue[3])
            return dither_color_blue[3];
        if (val <= dither_color_blue[0])
            return dither_color_blue[0];
        for (i = 0; i < 4; i++)
        {
            if (val < dither_color_blue[i])
                break;
            else if (val == dither_color_blue[i])
                return dither_color_blue[i];
        }

        return val - dither_color_blue[i - 1] < dither_color_blue[i] - val ? dither_color_blue[i - 1] : dither_color_blue[i];
       
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the average value of color in 5 * 5 area
//      
///////////////////////////////////////////////////////////////////////////////
int TargaImage::Box_Filter_Fetch_Value(int x, int y , int *swatches)
{
    float avg = 0;
    int first_index;
    int range;

    for (int dy = -2; dy <= 2; dy++)
    {
        first_index = (y + dy) * width;
        for (int dx = -2; dx <= 2; dx++)
        {
            range = Mask_Pos_In_Range(x + dx, y + dy);
            
            if (range == -1) // x and y are not both in range
            {
                avg += swatches[y * width + x] * 0.04;
            }

            else if (range == 0) // x is not in range
            {
                if (y + dy < 0)
                {
                    if(y)
                        avg += swatches[(y - 1) * width + (x + dx)] * 0.04;
                    else
                        avg += swatches[y * width + (x + dx)] * 0.04;
                }
                    
                else
                {
                    if (y == height - 2)
                        avg += swatches[(y + 1) * width + (x + dx)] * 0.04;
                    else
                        avg += swatches[y * width + (x + dx)] * 0.04;
                }
            }

            else if (range == 1) // y is not in range
            {
                if (x + dx < 0)
                {
                    if (x)
                        avg += swatches[(y + dy) * width + (x - 1)] * 0.04;
                    else
                        avg += swatches[(y + dy) * width + x] * 0.04;
                }

                else
                {
                    if (x == width - 2)
                        avg += swatches[(y + dy) * width + (x + 1)] * 0.04;
                    else
                        avg += swatches[(y + dy) * width + x] * 0.04;
                }
            }

            else // x and y are both in range
            {
                avg += swatches[first_index + x + dx] * 0.04;
            }
        }
    }

    return (int)avg;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the value of color in 5 * 5 area 
// with specific rate(Gaussian Filter)
//      
///////////////////////////////////////////////////////////////////////////////
int TargaImage::Gaussian_Filter_Fetch_Value(int x, int y, int* swatches)
{
    float avg = 0;
    int first_index;
    int range;

    for (int dy = -2, i = 0; dy <= 2; dy++, i++)
    {
        first_index = (y + dy) * width;

        for (int dx = -2, j = 0; dx <= 2; dx++, j++)
        {
            range = Mask_Pos_In_Range(x + dx, y + dy);

            if (range == -1) // x and y are not both in range
            {
                avg += (float)swatches[y * width + x] * (float)gaussian_filter_matrix[i][j] / 256.0f;
            }

            else if (range == 0) // x is not in range
            {
                if (y + dy < 0)
                {
                    if (y)
                        avg += (float)swatches[(y - 1) * width + (x + dx)] * (float)gaussian_filter_matrix[i][j] / 256.0f;
                    else
                        avg += (float)swatches[y * width + (x + dx)] * (float)gaussian_filter_matrix[i][j] / 256.0f;
                }

                else
                {
                    if (y == height - 2)
                        avg += (float)swatches[(y + 1) * width + (x + dx)] * (float)gaussian_filter_matrix[i][j] / 256.0f;
                    else
                        avg += (float)swatches[y * width + (x + dx)] * (float)gaussian_filter_matrix[i][j] / 256.0f;
                }
            }

            else if (range == 1) // y is not in range
            {
                if (x + dx < 0)
                {
                    if (x)
                        avg += (float)swatches[(y + dy) * width + (x - 1)] * (float)gaussian_filter_matrix[i][j] / 256.0f;
                    else
                        avg += (float)swatches[(y + dy) * width + x] * (float)gaussian_filter_matrix[i][j] / 256.0f;
                }

                else
                {
                    if (x == width - 2)
                        avg += (float)swatches[(y + dy) * width + (x + 1)] * (float)gaussian_filter_matrix[i][j] / 256.0f;
                    else
                        avg += (float)swatches[(y + dy) * width + x] * (float)gaussian_filter_matrix[i][j] / 256.0f;
                }
            }

            else // x and y are both in range
            {
                avg += (float)swatches[first_index + x + dx] * (float)gaussian_filter_matrix[i][j] / 256.0f;
            }
        }
    }

    return (int)avg;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Define the rule to sort pair<int, int> container, the bigger
//the second value, the more priority the piar<int, int> has
// 
///////////////////////////////////////////////////////////////////////////////
bool comp(pair<int, int> a, pair<int, int> b)
{
    return a.second > b.second;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Convert image to grayscale.  Red, green, and blue channels should all 
//  contain grayscale value.  Alpha channel shoould be left unchanged.  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::To_Grayscale()
{
    float Y;
    for (int i = 0; i < data_array_size; i += 4)
    {
        Y = 0.3 * (float)data[i] + 0.59 * (float)data[i + 1] + 0.11 * (float)data[i + 2];
        data[i] = data[i + 1] = data[i + 2] = Y;
    }
    return true;
}// To_Grayscale


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using uniform quantization.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Uniform()
{
    for (int i = 0; i < data_array_size; i+=4)
    {
        data[i]      =  (data[i] & 224);
        data[i + 1]  =  (data[i + 1] & 224);
        data[i + 2]  =  (data[i + 2] & 192);
    }
    return true;
}// Quant_Uniform


///////////////////////////////////////////////////////////////////////////////
//
//      Convert the image to an 8 bit image using populosity quantization.  
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Populosity()
{
    pair<int, int>* color_number  =  new pair<int, int>[32769];
    Color* pop_color              =  new Color[256];
    int index;

    for (int i = 0; i < 32768; i++)
    {
        color_number[i].first   =  i;
        color_number[i].second  =  0;
    }

    for (int i = 0; i < data_array_size; i += 4)
    {
        int index = (int)((data[i] >> 3) << 10) + (int)((data[i + 1] >> 3) << 5) + (int)(data[i + 2] >> 3);
        color_number[index].second++;
    }

    sort(color_number, color_number + 32769, comp);

    for (int i = 0; i < 256; i++)
    {
        pop_color[i].red    =  (color_number[i].first >> 10) << 3;
        pop_color[i].green  =  ((color_number[i].first % 1024) >> 5) <<3;
        pop_color[i].blue   =  (color_number[i].first % 32) << 3;
    }

    int distance;
    int d_r, d_g, d_b;
    int dis_min;
    int min_index;


    for (int i = 0; i < data_array_size; i += 4)
    {
        dis_min    =  200000;
        min_index  =  1000;
        for (int j = 0; j < 256; j++)
        {
            d_r  =  pop_color[j].red - data[i];
            d_g  =  pop_color[j].green - data[i + 1];
            d_b  =  pop_color[j].blue - data[i + 2];

            distance = d_r * d_r + d_g * d_g + d_b * d_b;

            if (dis_min > distance)
            {
                dis_min = distance;
                min_index = j;
            }
        }

        data[i]      =  pop_color[min_index].red;
        data[i + 1]  =  pop_color[min_index].green;
        data[i + 2]  =  pop_color[min_index].blue;
    }

    delete [] color_number;
    return true;
}// Quant_Populosity


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image using a threshold of 1/2.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Threshold()
{
    To_Grayscale();
    for (int i = 0; i < data_array_size; i += 4)
    {
        if (data[i] >= 127)
            data[i] = data[i + 1] = data[i + 2] = BRIGHT;
        else
            data[i] = data[i + 1] = data[i + 2] = DARK;
    }
    return true;
}// Dither_Threshold


///////////////////////////////////////////////////////////////////////////////
//
//      Dither image using random dithering.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Random()
{
    srand(time(NULL));
    To_Grayscale();
    float rand_num;

    for (int i = 0; i < data_array_size; i += 4)
    {
        rand_num = (float)(rand() % 401 - 200)/1000.0;
        data[i] = data[i] + 255.0f * rand_num;
        if (data[i] >= 127)
            data[i] = data[i + 1] = data[i + 2] = BRIGHT;
        else
            data[i] = data[i + 1] = data[i + 2] = DARK;
    }
    return true;
}// Dither_Random


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS()
{
    To_Grayscale();
    int prev_row = 0;
    int index = 0;
    int move_index = 0;
    float error = 0;
    float* temp_img = new float[img_size];

    for (int i = 0, j = 0; i < data_array_size; i += 4, j++)
        temp_img[j] = data[i];

    for (int y = 0; y < height; y++)
    {
        prev_row = y * width;
        // Odd row must move from right to left side
        if (y % 2)
        {
            for (int x = width - 1; x >= 0; x--)
            {
                index = prev_row + x;
                if (temp_img[index] >= 127)
                {
                    error = temp_img[index] - 255.0f;
                    temp_img[index] = 255;
                }
                else
                {
                    error = temp_img[index];
                    temp_img[index] = 0;
                }

                for (int i = 0; i < 4; i++)
                {
                    if (Is_Valid_Img_Pos(x + dither_fs_move_odd[i][0], y + dither_fs_move_odd[i][1]))
                    {
                        move_index = index + width * dither_fs_move_odd[i][1] + dither_fs_move_odd[i][0];
                        temp_img[move_index] += (error * dither_fs_error_rate[i]);
                    }
                }
            }
        }

        // Even row must move from left to right side
        else
        {
            for (int x = 0; x < width; x++)
            {
                index = prev_row + x;
                if (temp_img[index] >= 127)
                {
                    error = temp_img[index] - 255.0f;
                    temp_img[index] = 255;
                }
                else
                {
                    error = temp_img[index];
                    temp_img[index] = 0;
                }

                for (int i = 0; i < 4; i++)
                {
                    if (Is_Valid_Img_Pos(x + dither_fs_move_even[i][0], y + dither_fs_move_even[i][1]))
                    {
                        move_index = index + width * dither_fs_move_even[i][1] + dither_fs_move_even[i][0];
                        temp_img[move_index] += (error * dither_fs_error_rate[i]);
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < data_array_size; i += 4)
    {
        data[i] = data[i + 1] = data[i + 2] = (temp_img[i/4] >= 127 ? 255: 0);
    }
    delete[]temp_img;
    return true;
}// Dither_FS


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the swatches.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS_Swatches(float *swatches, int type)
{
    int prev_row = 0;
    int index = 0;
    int move_index = 0;
    float error = 0;
    
    for (int y = 0; y < height; y++)
    {
        prev_row = y * width;
        // Odd row must move from right to left side
        if (y % 2)
        {
            for (int x = width - 1; x >= 0; x--)
            {
                index = prev_row + x;
                error = swatches[index];
                swatches[index] = (float)Find_Proper_Dither_Color(type, swatches[index]);
                error = error - swatches[index];

                for (int i = 0; i < 4; i++)
                {
                    if (Is_Valid_Img_Pos(x + dither_fs_move_odd[i][0], y + dither_fs_move_odd[i][1]))
                    {
                        move_index = index + width * dither_fs_move_odd[i][1] + dither_fs_move_odd[i][0];
                        swatches[move_index] += (error * dither_fs_error_rate[i]);
                        if (swatches[move_index] <= 0)
                            swatches[move_index] = 0;
                        else if (swatches[move_index] > 255)
                            swatches[move_index] = 255;
                    }
                }
            }
        }

        // Even row must move from left to right side
        else
        {
            for (int x = 0; x < width; x++)
            {
                index = prev_row + x;
                error = swatches[index];
                swatches[index] = (float)Find_Proper_Dither_Color(type, swatches[index]);
                error = error - swatches[index];

                for (int i = 0; i < 4; i++)
                {
                    if (Is_Valid_Img_Pos(x + dither_fs_move_even[i][0], y + dither_fs_move_even[i][1]))
                    {
                        move_index = index + width * dither_fs_move_even[i][1] + dither_fs_move_even[i][0];
                        swatches[move_index] += (error * dither_fs_error_rate[i]);
                        if (swatches[move_index] <= 0)
                            swatches[move_index] = 0;
                        else if (swatches[move_index] > 255)
                            swatches[move_index] = 255;
                    }
                }
            }
        }
    }
    return true;
}// Dither_FS_Swatches


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image while conserving the average brightness.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Bright()
{
    To_Grayscale();
    int sum = 0;
    int bright_number;
    int turning_point;
    float britness;
    float threshold;
    int pixel_intensity[256] = {};

    for (int i = 0; i < data_array_size; i+=4)
        sum += data[i];
       
    britness = (float)sum / img_size / 255.0f;
    
    for (int i = 0; i < data_array_size; i += 4)
        pixel_intensity[data[i]]++;

    int dark_number = (1-britness) * img_size;

    turning_point = 0;
    while (turning_point < 256)
    {
        dark_number -= pixel_intensity[turning_point];
        if (dark_number <= 0)
            break;
        turning_point++;
    }

    if (turning_point == 256)
        threshold = 1.0f;
    else
        threshold = (float)turning_point / 255.0f;

    threshold *= 255.0f;

    for (int i = 0; i < data_array_size; i += 4)
    {
        if (data[i] >= threshold)
            data[i] = data[i + 1] = data[i + 2] = BRIGHT;
        else
            data[i] = data[i + 1] = data[i + 2] = DARK;
    }

    return true;
}// Dither_Bright


///////////////////////////////////////////////////////////////////////////////
//
//      Perform clustered differing of the image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Cluster()
{
    int first;
    To_Grayscale();
    for (int y = 0; y < height; y++)
    {
        first = y * width * 4;
        for (int x = 0; x < width * 4; x += 4)
        {
            if ((float)data[first + x] / 255.0 >= cluster_matrix[y % 4][(x/4) % 4])
                data[first + x] = data[first + x + 1] = data[first + x + 2] = BRIGHT;
            else
                data[first + x] = data[first + x + 1] = data[first + x + 2] = DARK;
        }
    }
    return true;
}// Dither_Cluster


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using Floyd-Steinberg dithering over
//  a uniform quantization - the same quantization as in Quant_Uniform.
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Color()
{
    float* r_swatches  =  new float[img_size];
    float* g_swatches  =  new float[img_size];
    float* b_swatches  =  new float[img_size];
    

    for (int i = 0; i < data_array_size; i += 4)
    {
        r_swatches[i / 4]  =  data[i];
        g_swatches[i / 4]  =  data[i + 1];
        b_swatches[i / 4]  =  data[i + 2];
    }

    Dither_FS_Swatches(r_swatches, 0);
    Dither_FS_Swatches(g_swatches, 1);
    Dither_FS_Swatches(b_swatches, 2);
    
    for (int i = 0; i < data_array_size; i += 4)
    {
        data[i]      =  (int)r_swatches[i / 4];
        data[i + 1]  =  (int)g_swatches[i / 4];
        data[i + 2]  =  (int)b_swatches[i / 4];
    }
    delete [] r_swatches, delete [] b_swatches, delete [] g_swatches;
    return true;
}// Dither_Color


///////////////////////////////////////////////////////////////////////////////
//
//      Composite the current image over the given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Over(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout <<  "Comp_Over: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Over


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "in" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_In(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_In: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_In


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "out" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Out(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Out: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Out


///////////////////////////////////////////////////////////////////////////////
//
//      Composite current image "atop" given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Atop(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Atop: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Atop


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image with given image using exclusive or (XOR).  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Xor(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Xor: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Xor


///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the difference bewteen this imag and the given one.  Image 
//  dimensions must be equal.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Difference(TargaImage* pImage)
{
    if (!pImage)
        return false;

    if (width != pImage->width || height != pImage->height)
    {
        cout << "Difference: Images not the same size\n";
        return false;
    }// if

    for (int i = 0 ; i < width * height * 4 ; i += 4)
    {
        unsigned char        rgb1[3];
        unsigned char        rgb2[3];

        RGBA_To_RGB(data + i, rgb1);
        RGBA_To_RGB(pImage->data + i, rgb2);

        data[i] = abs(rgb1[0] - rgb2[0]);
        data[i+1] = abs(rgb1[1] - rgb2[1]);
        data[i+2] = abs(rgb1[2] - rgb2[2]);
        data[i+3] = 255;
    }

    return true;
}// Difference


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 box filter on this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Box()
{
    int* r_swatches         =  new int[img_size];
    int* g_swatches         =  new int[img_size];
    int* b_swatches         =  new int[img_size];
    int** fetch_near_color  =  new int* [5];
    int fir_index;

    for (int i = 0; i < 5; i++)
        fetch_near_color[i] = new int[5];

    for (int i = 0; i < data_array_size; i += 4)
    {
        r_swatches[i / 4] =  data[i];
        g_swatches[i / 4] =  data[i + 1];
        b_swatches[i / 4] =  data[i + 2];
    }

    for (int y = 0; y < height; y++)
    {
        fir_index = y * width * 4;
        for (int x = 0; x < width; x++)
        {
            data[fir_index + x * 4]      =  Box_Filter_Fetch_Value(x, y, r_swatches);
            data[fir_index + x * 4 + 1]  =  Box_Filter_Fetch_Value(x, y, g_swatches);
            data[fir_index + x * 4 + 2]  =  Box_Filter_Fetch_Value(x, y, b_swatches);
        }
    }

    for (int i = 0; i < 5; i++)
        delete fetch_near_color[i];
    delete [] fetch_near_color;
    
    delete [] r_swatches, delete [] b_swatches, delete [] g_swatches;
    return true;
}// Filter_Box

///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the value of color in 5 * 5 area 
// with specific rate(Bartlett Filter)
//      
///////////////////////////////////////////////////////////////////////////////
int TargaImage::Bartlett_Filter_Fetch_Value(int x, int y, int* swatches)
{
    float avg = 0;
    int first_index;
    int range;

    for (int dy = -2, i = 0; dy <= 2; dy++, i++)
    {
        first_index = (y + dy) * width;

        for (int dx = -2, j = 0; dx <= 2; dx++, j++)
        {
            range = Mask_Pos_In_Range(x + dx, y + dy);

            if (range == -1) // x and y are not both in range
            {
                avg += (float)swatches[y * width + x] * (float)bartlett_filter_matrix[i][j] / 81.0f;
            }

            else if (range == 0) // x is not in range
            {
                if (y + dy < 0)
                {
                    if (y)
                        avg += (float)swatches[(y - 1) * width + (x + dx)] * (float)bartlett_filter_matrix[i][j] / 81.0f;
                    else
                        avg += (float)swatches[y * width + (x + dx)] * (float)bartlett_filter_matrix[i][j] / 81.0f;
                }

                else
                {
                    if (y == height - 2)
                        avg += (float)swatches[(y + 1) * width + (x + dx)] * (float)bartlett_filter_matrix[i][j] / 81.0f;
                    else
                        avg += (float)swatches[y * width + (x + dx)] * (float)bartlett_filter_matrix[i][j] / 81.0f;
                }
            }

            else if (range == 1) // y is not in range
            {
                if (x + dx < 0)
                {
                    if (x)
                        avg += (float)swatches[(y + dy) * width + (x - 1)] * (float)bartlett_filter_matrix[i][j] / 81.0f;
                    else
                        avg += (float)swatches[(y + dy) * width + x] * (float)bartlett_filter_matrix[i][j] / 81.0f;
                }

                else
                {
                    if (x == width - 2)
                        avg += (float)swatches[(y + dy) * width + (x + 1)] * (float)bartlett_filter_matrix[i][j] / 81.0f;
                    else
                        avg += (float)swatches[(y + dy) * width + x] * (float)bartlett_filter_matrix[i][j] / 81.0f;
                }
            }

            else // x and y are both in range
            {
                avg += (float)swatches[first_index + x + dx] * (float)bartlett_filter_matrix[i][j] / 81.0f;
            }
        }
    }

    return (int)avg;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett()
{
    int* r_swatches         =  new int[img_size];
    int* g_swatches         =  new int[img_size];
    int* b_swatches         =  new int[img_size];
    int** fetch_near_color  =  new int* [5];
    int fir_index;

    for (int i = 0; i < 5; i++)
        fetch_near_color[i] = new int[5];

    for (int i = 0; i < data_array_size; i += 4)
    {
        r_swatches[i / 4] = data[i];
        g_swatches[i / 4] = data[i + 1];
        b_swatches[i / 4] = data[i + 2];
    }

    for (int y = 0; y < height; y++)
    {
        fir_index = y * width * 4;

        for (int x = 0; x < width; x++)
        {
            data[fir_index + x * 4]      =  Bartlett_Filter_Fetch_Value(x, y, r_swatches);
            data[fir_index + x * 4 + 1]  =  Bartlett_Filter_Fetch_Value(x, y, g_swatches);
            data[fir_index + x * 4 + 2]  =  Bartlett_Filter_Fetch_Value(x, y, b_swatches);
        }
    }

    for (int i = 0; i < 5; i++)
        delete fetch_near_color[i];
    delete [] fetch_near_color;

    delete [] r_swatches, delete [] b_swatches, delete [] g_swatches;
    return true;
}// Filter_Bartlett

///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the value of color in n * m area 
// with specific rate(Bartlett Filter)
//      
///////////////////////////////////////////////////////////////////////////////
int TargaImage::Bartlett_Filter_NM_Fetch_Value(int x, int y, int* swatches, int n, int m, int type, float base)
{
    float avg = 0;
    int first_index;
    int range;
    int firsty, endy;
    int firstx, endx;
    firsty = n % 2 ? -n / 2 : -(n - 1) / 2;
    endy = n / 2;
    firstx = m % 2 ? -m / 2 : -(m - 1) / 2;
    endx = m / 2;


    for (int dy = firsty, i = 0; dy <= endy; dy++, i++)
    {
        first_index = (y + dy) * width;

        for (int dx = firstx, j = 0; dx <= endx; dx++, j++)
        {
            range = Mask_Pos_In_Range(x + dx, y + dy);

            if (range == -1) // x and y are not both in range
            {
                avg += (float)swatches[y * width + x] * Find_Matrix_Val_With_Type(j, i, type) / base;
            }

            else if (range == 0) // x is not in range
            {
                if (y + dy < 0)
                {
                    if (y)
                        avg += (float)swatches[(y - 1) * width + (x + dx)] * Find_Matrix_Val_With_Type(j, i, type) / base;
                    else
                        avg += (float)swatches[y * width + (x + dx)] * Find_Matrix_Val_With_Type(j, i, type) / base;
                }

                else
                {
                    if (y == height - 2)
                        avg += (float)swatches[(y + 1) * width + (x + dx)] * Find_Matrix_Val_With_Type(j, i, type) / base;
                    else
                        avg += (float)swatches[y * width + (x + dx)] * Find_Matrix_Val_With_Type(j, i, type) / base;
                }
            }

            else if (range == 1) // y is not in range
            {
                if (x + dx < 0)
                {
                    if (x)
                        avg += (float)swatches[(y + dy) * width + (x - 1)] * Find_Matrix_Val_With_Type(j, i, type) / base;
                    else
                        avg += (float)swatches[(y + dy) * width + x] * Find_Matrix_Val_With_Type(j, i, type) / base;
                }

                else
                {
                    if (x == width - 2)
                        avg += (float)swatches[(y + dy) * width + (x + 1)] * Find_Matrix_Val_With_Type(j, i, type) / base;
                    else
                        avg += (float)swatches[(y + dy) * width + x] * Find_Matrix_Val_With_Type(j, i, type) / base;
                }
            }

            else // x and y are both in range
            {
                avg += (float)swatches[first_index + x + dx] * Find_Matrix_Val_With_Type(j, i, type) / base;
            }
        }
    }

    return avg > 255 ? 255 : (int)avg;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Perform n * m Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett_NM(int type, int n, int m, float base)
{
    int* r_swatches = new int[img_size];
    int* g_swatches = new int[img_size];
    int* b_swatches = new int[img_size];
    int fir_index;


    for (int i = 0; i < data_array_size; i += 4)
    {
        r_swatches[i / 4] = data[i];
        g_swatches[i / 4] = data[i + 1];
        b_swatches[i / 4] = data[i + 2];
    }

    for (int y = 0; y < height; y++)
    {
        fir_index = y * width * 4;

        for (int x = 0; x < width; x++)
        {
            data[fir_index + x * 4]     = Bartlett_Filter_NM_Fetch_Value(x, y, r_swatches, n, m, type, base);
            data[fir_index + x * 4 + 1] = Bartlett_Filter_NM_Fetch_Value(x, y, g_swatches, n, m, type, base);
            data[fir_index + x * 4 + 2] = Bartlett_Filter_NM_Fetch_Value(x, y, b_swatches, n, m, type, base);
        }
    }

    delete[] r_swatches, delete[] b_swatches, delete[] g_swatches;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Gaussian()
{
    int* r_swatches = new int[img_size];
    int* g_swatches = new int[img_size];
    int* b_swatches = new int[img_size];
    int** fetch_near_color = new int* [5];
    int fir_index;

    for (int i = 0; i < 5; i++)
        fetch_near_color[i] = new int[5];

    for (int i = 0; i < data_array_size; i += 4)
    {
        r_swatches[i / 4] = data[i];
        g_swatches[i / 4] = data[i + 1];
        b_swatches[i / 4] = data[i + 2];
    }

    for (int y = 0; y < height; y++)
    {
        fir_index = y * width * 4;

        for (int x = 0; x < width; x++)
        {
            data[fir_index + x * 4]     = Gaussian_Filter_Fetch_Value(x, y, r_swatches);
            data[fir_index + x * 4 + 1] = Gaussian_Filter_Fetch_Value(x, y, g_swatches);
            data[fir_index + x * 4 + 2] = Gaussian_Filter_Fetch_Value(x, y, b_swatches);
        }
    }

    for (int i = 0; i < 5; i++)
        delete fetch_near_color[i];
    delete[] fetch_near_color;

    delete[] r_swatches, delete[] b_swatches, delete[] g_swatches;
    return true;
}// Filter_Gaussian

///////////////////////////////////////////////////////////////////////////////
//
//      Perform NxN Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Gaussian_N( unsigned int N )
{
    ClearToBlack();
   return false;
}// Filter_Gaussian_N


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 edge detect (high pass) filter on this image.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Edge()
{
    ClearToBlack();
    return false;
}// Filter_Edge


///////////////////////////////////////////////////////////////////////////////
//
//      Perform a 5x5 enhancement filter to this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Enhance()
{
    ClearToBlack();
    return false;
}// Filter_Enhance


///////////////////////////////////////////////////////////////////////////////
//
//      Run simplified version of Hertzmann's painterly image filter.
//      You probably will want to use the Draw_Stroke funciton and the
//      Stroke class to help.
// Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::NPR_Paint()
{
    ClearToBlack();
    return false;
}


///////////////////////////////////////////////////////////////////////////////
//
//      Halve the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Half_Size()
{
    Filter_Bartlett_NM(1, 3, 3, 16);

    int* half_img = new int[img_size]();
    int fir_index, tfir_index;

    int new_index = 0;
    for (int y = 0; y < height; y ++)
    {
        fir_index = y * width * 4;

        for (int x = 0; x < width; x ++)
        {
            if (!(x % 2) || !(y % 2))
                continue;

            tfir_index = fir_index + x * 4;

            half_img[new_index  ]  =  data[tfir_index    ];
            half_img[++new_index]  =  data[tfir_index + 1];
            half_img[++new_index]  =  data[tfir_index + 2];
            half_img[++new_index]  =  255;
            new_index++;
        }
    }

    delete [] data;
    data = new unsigned char[img_size]();

    for (int i = 0; i < img_size; i++)
        data[i] = half_img[i];

    width /= 2;
    height /= 2;
    img_size = width * height;
    data_array_size = img_size * 4;

    delete [] half_img;
    return true;
}// Half_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Double the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Double_Size()
{ 
    int* r_swatches = new int[img_size];
    int* g_swatches = new int[img_size];
    int* b_swatches = new int[img_size];

    for (int i = 0; i < data_array_size; i += 4)
    {
        r_swatches[i / 4] = data[i];
        g_swatches[i / 4] = data[i + 1];
        b_swatches[i / 4] = data[i + 2];
    }

    data_array_size *= 4;

    int* double_img = new int[data_array_size]();
    int new_img_index, old_img_index;
    int tnew_img_index, told_img_index;
    int double_width = width * 2;
    int double_height = height * 2;
    int red_val, green_val, blue_val;

    for (int y = 0; y < double_height; y++)
    {
        new_img_index = y * double_width * 4;
        old_img_index = width * (y / 2) * 4;

        for (int x = 0; x < double_width; x++)
        {
            tnew_img_index = new_img_index + 4 * x;
            told_img_index = old_img_index + (x / 2) * 4;

            if (y % 2 && x % 2)
            {
                red_val = Bartlett_Filter_NM_Fetch_Value(x / 2, y / 2, r_swatches, 3, 3, 2, 16);
                green_val = Bartlett_Filter_NM_Fetch_Value(x / 2, y / 2, g_swatches, 3, 3, 2, 16);
                blue_val = Bartlett_Filter_NM_Fetch_Value(x / 2, y / 2, b_swatches, 3, 3, 2, 16);
            }

            else if (y % 2 || x % 2)
            {
                red_val = Bartlett_Filter_NM_Fetch_Value(x / 2, y / 2, r_swatches, 4, 3, 4, 32);
                green_val = Bartlett_Filter_NM_Fetch_Value(x / 2, y / 2, g_swatches, 4, 3, 4, 32);
                blue_val = Bartlett_Filter_NM_Fetch_Value(x / 2, y / 2, b_swatches, 4, 3, 4, 32);
            }

            else
            {
                red_val = Bartlett_Filter_NM_Fetch_Value(x / 2, y / 2, r_swatches, 4, 4, 3, 64);
                green_val = Bartlett_Filter_NM_Fetch_Value(x / 2, y / 2, g_swatches, 4, 4, 3, 64);
                blue_val = Bartlett_Filter_NM_Fetch_Value(x / 2, y / 2, b_swatches, 4, 4, 3, 64);
            }

            double_img[tnew_img_index    ]  =  red_val;
            double_img[tnew_img_index + 1]  =  green_val;
            double_img[tnew_img_index + 2]  =  blue_val;
            double_img[tnew_img_index + 3]  =  255;
        }
    }

    width = double_width;
    height = double_height;
    img_size *= 4;

    delete [] data;
    data = new unsigned char[data_array_size];

    for (int i = 0; i < data_array_size; i++)
        data[i] = double_img[i];

    return true;
}// Double_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Scale the image dimensions by the given factor.  The given factor is 
//  assumed to be greater than one.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Resize(float scale)
{
    ClearToBlack();
    return false;
}// Resize


//////////////////////////////////////////////////////////////////////////////
//
//      Rotate the image clockwise by the given angle.  Do not resize the 
//  image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Rotate(float angleDegrees)
{
    ClearToBlack();
    return false;
}// Rotate


//////////////////////////////////////////////////////////////////////////////
//
//      Given a single RGBA pixel return, via the second argument, the RGB
//      equivalent composited with a black background.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::RGBA_To_RGB(unsigned char *rgba, unsigned char *rgb)
{
    const unsigned char	BACKGROUND[3] = { 0, 0, 0 };

    unsigned char  alpha = rgba[3];

    if (alpha == 0)
    {
        rgb[0] = BACKGROUND[0];
        rgb[1] = BACKGROUND[1];
        rgb[2] = BACKGROUND[2];
    }
    else
    {
	    float	alpha_scale = (float)255 / (float)alpha;
	    int	val;
	    int	i;

	    for (i = 0 ; i < 3 ; i++)
	    {
	        val = (int)floor(rgba[i] * alpha_scale);
	        if (val < 0)
		    rgb[i] = 0;
	        else if (val > 255)
		    rgb[i] = 255;
	        else
		    rgb[i] = val;
	    }
    }
}// RGA_To_RGB


///////////////////////////////////////////////////////////////////////////////
//
//      Copy this into a new image, reversing the rows as it goes. A pointer
//  to the new image is returned.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Reverse_Rows(void)
{
    unsigned char   *dest = new unsigned char[width * height * 4];
    TargaImage	    *result;
    int 	        i, j;

    if (! data)
    	return NULL;

    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = (height - i - 1) * width * 4;
	    int out_offset = i * width * 4;

	    for (j = 0 ; j < width ; j++)
        {
	        dest[out_offset + j * 4] = data[in_offset + j * 4];
	        dest[out_offset + j * 4 + 1] = data[in_offset + j * 4 + 1];
	        dest[out_offset + j * 4 + 2] = data[in_offset + j * 4 + 2];
	        dest[out_offset + j * 4 + 3] = data[in_offset + j * 4 + 3];
        }
    }

    result = new TargaImage(width, height, dest);
    delete[] dest;
    return result;
}// Reverse_Rows


///////////////////////////////////////////////////////////////////////////////
//
//      Clear the image to all black.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::ClearToBlack()
{
    memset(data, 0, width * height * 4);
}// ClearToBlack


///////////////////////////////////////////////////////////////////////////////
//
//      Helper function for the painterly filter; paint a stroke at
// the given location
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::Paint_Stroke(const Stroke& s) {
   int radius_squared = (int)s.radius * (int)s.radius;
   for (int x_off = -((int)s.radius); x_off <= (int)s.radius; x_off++) {
      for (int y_off = -((int)s.radius); y_off <= (int)s.radius; y_off++) {
         int x_loc = (int)s.x + x_off;
         int y_loc = (int)s.y + y_off;
         // are we inside the circle, and inside the image?
         if ((x_loc >= 0 && x_loc < width && y_loc >= 0 && y_loc < height)) {
            int dist_squared = x_off * x_off + y_off * y_off;
            if (dist_squared <= radius_squared) {
               data[(y_loc * width + x_loc) * 4 + 0] = s.r;
               data[(y_loc * width + x_loc) * 4 + 1] = s.g;
               data[(y_loc * width + x_loc) * 4 + 2] = s.b;
               data[(y_loc * width + x_loc) * 4 + 3] = s.a;
            } else if (dist_squared == radius_squared + 1) {
               data[(y_loc * width + x_loc) * 4 + 0] = 
                  (data[(y_loc * width + x_loc) * 4 + 0] + s.r) / 2;
               data[(y_loc * width + x_loc) * 4 + 1] = 
                  (data[(y_loc * width + x_loc) * 4 + 1] + s.g) / 2;
               data[(y_loc * width + x_loc) * 4 + 2] = 
                  (data[(y_loc * width + x_loc) * 4 + 2] + s.b) / 2;
               data[(y_loc * width + x_loc) * 4 + 3] = 
                  (data[(y_loc * width + x_loc) * 4 + 3] + s.a) / 2;
            }
         }
      }
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke() {}

///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke(unsigned int iradius, unsigned int ix, unsigned int iy,
               unsigned char ir, unsigned char ig, unsigned char ib, unsigned char ia) :
   radius(iradius),x(ix),y(iy),r(ir),g(ig),b(ib),a(ia)
{
}

