/*
 * This program is free software licensed under GNU GPL version 3 or later
 * 
 * (c) 2015 Felipe Correa da Silva Sanches <juca@members.fsf.org>
 * 
 */

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>

using namespace cv;
#define COLOR_RED Scalar(0, 0, 255)
#define COLOR_GREEN Scalar(0, 255, 0)
#define COLOR_BLUE Scalar(255, 0, 0)
#define COLOR_YELLOW Scalar(0, 255, 255)
#define COLOR_GREY Scalar(128, 128, 128)

#define PROGRAM_NAME "Dump data from punched tapes"
#define MAX_DATA_SIZE 256
unsigned char data[MAX_DATA_SIZE];
bool inverted = true; //TODO: command line switch to select 
                      //      whether or not the tape is upside-down
bool exchange_points = false;
unsigned char BIT_MASK = 0xFF;

// These vectors will be used to store the coordinates and radius of
// the detected holes in the scanned image of the punched tape.
std::vector<Vec3f> reference_dots, bits;

void set_bit(int addr, int bit){
    //do not attempt to write data off-bounds
    if (addr < 0 || addr > MAX_DATA_SIZE)
        return;

    //compensate for the extra spacing
    //due to the reference dots
    if (bit > 5) {
        bit--;
    }

    //turn on the specified bit
    data[addr] |= (1 << (7-bit));
}

void init_tape_scanner(){
    /*******************************************************************
     * This program will analize an image of a scanned punched         *
     * tape and will detect the patterns of holes in order to          *
     * generate an output ROM file with the data contents originaly    *
     * stored in the tape.                                             *
     *******************************************************************/

    //init data with zeroes
    for (int i=0; i < MAX_DATA_SIZE; i++){
        data[i] = 0;
    }
}
// TODO: Break it down into smaller functions, each responsible for a specific subtask.
void process_frame(Mat src, bool with_pauses){
    Mat src_gray;

    imshow(PROGRAM_NAME, src);
    if (with_pauses) waitKey(0);

    // Convert it to gray
    cvtColor(src, src_gray, COLOR_BGR2GRAY);

    imshow(PROGRAM_NAME, src_gray);
    if (with_pauses) waitKey(0);

    // Reduce the noise so we avoid false circle detection
    GaussianBlur(src_gray, src_gray, Size(9, 9), 2, 2);

    imshow(PROGRAM_NAME, src_gray);
    if (with_pauses) waitKey(0);

    /****************************************************
     * Detect geometric elements in the provided image. *
     ****************************************************/

    // Apply the Hough Transform to find the circles
    //TODO: Document the meaning of the parameters in these function calls.
    //TODO: Perhaps add UI controls or command-line options
    //      to tweak these parameters of the Hough Transforms.
    HoughCircles(src_gray, reference_dots, HOUGH_GRADIENT, 1, 10, 200, 15, 0, 7); // The reference dots are smaller
    HoughCircles(src_gray, bits, HOUGH_GRADIENT, 1, 10, 200, 15, 9, 30); // And the bit holes are a bit larger

    /************************************************************
     * Detect the regularity of reference holes so that we      *
     * can extract the pitch distance of the tape and correctly *
     * interpret the data patterns in the tape grid.            *
     ************************************************************/

    // Points A and B are the couple of reference dots which
    // are the farthest away from each other.
    // Their positions are used to infer the inclination of the tape in
    // the scanned image. Their distance (when divided by the tape pitch) is used
    // to calculate how many bytes are stored in the punched tape.
    Point A, B;

    double new_distance;  
    double max_distance = 0;
    double min_distance;
    for (size_t i = 0; i < reference_dots.size() - 1; i++){
        for (size_t j = i+1; j < reference_dots.size(); j++){
            Point I(reference_dots[i][0], reference_dots[i][1]);
            Point J(reference_dots[j][0], reference_dots[j][1]);
            new_distance = cv::norm(I - J);

            if (i==0 && j==1)
                min_distance = new_distance;

            if (new_distance < min_distance)
                min_distance = new_distance;

            if (new_distance > max_distance){
                max_distance = new_distance;
                A = I;
                B = J;
            }
        }
    }

    float pitch = 0;
    int count = 0;
    
    for (size_t i = 0; i < reference_dots.size() - 1; i++){
        for (size_t j = i+1; j < reference_dots.size(); j++){
            Point I(reference_dots[i][0], reference_dots[i][1]);
            Point J(reference_dots[j][0], reference_dots[j][1]);
            new_distance = cv::norm(I - J);

            if (new_distance < 1.5 * min_distance){
                pitch += new_distance;
                count ++;
            }
        }
    }
    pitch /= count;
    
    // The number of rows of data detected
    // in the scanned image of the punched tape.
    int rowsOfData = floor(max_distance / pitch) + 1;
    printf("Rows of Data = %d\n", rowsOfData);

    int radius;
    for (size_t i = 0; i < reference_dots.size(); i++){
        Point center(cvRound(reference_dots[i][0]), cvRound(reference_dots[i][1]));
        radius = cvRound(reference_dots[i][2]);
        circle(src, center, radius, COLOR_YELLOW, 3, 8, 0);
    }

    imshow(PROGRAM_NAME, src);
    if (with_pauses) waitKey(0);

    for (size_t i = 0; i < bits.size(); i++){
        Point center(cvRound(bits[i][0]), cvRound(bits[i][1]));
        radius = cvRound(bits[i][2]);
        circle(src, center, radius, COLOR_GREEN, 3, 8, 0);
    }

    imshow(PROGRAM_NAME, src);
    if (with_pauses) waitKey(0);

    radius = 8;
    for (int i = 0; i < rowsOfData; i++){
        Point center = A + (((float) i) / (rowsOfData-1)) * (B - A);
        circle(src, center, radius, COLOR_RED, 3, 8, 0);
    }

    imshow(PROGRAM_NAME, src);
    if (with_pauses) waitKey(0);

    if (exchange_points){
        Point tmp = A;
        A = B;
        B = tmp;        
    }

    Point ortho((B - A).y, -(B - A).x); //AB rotated 90 degrees

    radius = 8;
    double fontScale = 1;
    int baseline=0;
    int fontFace=1;
    double thickness=2;
    for (int i = 0; i < rowsOfData; i++){
        Point center = A + (((float) i) / (rowsOfData-1)) * (B - A) - (5.8*pitch/cv::norm(ortho))*ortho;
        char text[3];
        sprintf(text, "%02X", i);
        Size textSize = getTextSize(text, fontFace, fontScale, thickness, &baseline);

        // center the text
        Point textOffset((-textSize.width)/2,
                         (textSize.height)/2);
        
        putText(src, text, center+textOffset, fontFace, fontScale, COLOR_RED, thickness, /* lineType */8, /* bottomLeftOrigin */ false);
    }

    imshow(PROGRAM_NAME, src);
    if (with_pauses) waitKey(0);

#if 0
    /* This is the new implementation, but I was not yet able to make it work... */

    // The following (U,V) unit vectors define a new coordinate system
    // which is aligned with the punched tape grid of holes
    Point U = (1.0 / cv::norm(B - A)) * (B - A);
    Point V(U.y, -U.x); //vector V is the result of
                        //rotating vector U 90 degrees clockwise

    //row and bit coordinates bellow are in the (U,V) system:
    for (size_t i = 0; i < bits.size(); i++){
        Point bit_vector = Point(cvRound(bits[i][0]), cvRound(bits[i][1])) - A;

        //Pitch is the spacing between neighbour holes in the tape.
        //The unit vector U is aligned to the reference holes, so
        // this means that the x integer coordinate in the tape grid
        // corresponds to memory rows (i.e. addresses).
        int row = floor(cv::norm(bit_vector.dot(U)) / pitch + 0.5);
        
        //The unit vector V is aligned to the bits in any given memory address
        // We shift the bit_vector by 5 grid units along the V axis
        // so that the amplitude of its projection in the V direction
        // reflects the bit position within the number stored in that memory row
        int bit = floor(cv::norm((bit_vector + 5 * pitch * V).dot(V)) / pitch + 0.5);
        set_bit(row, bit);
    }
#else
    /* This is the old implementation that works... */

    for (size_t i = 0; i < bits.size(); i++){
        Point bit_vector = Point(cvRound(bits[i][0]), cvRound(bits[i][1])) - A;
        double x = cv::norm(bit_vector.dot(B - A) / cv::norm(B - A));
        double y = cv::norm((bit_vector + (5*pitch/cv::norm(ortho))*ortho).dot(ortho) / cv::norm(ortho));
        set_bit((int) floor(x/pitch + 0.5), (int) floor(y/pitch + 0.5));
    }
#endif
    
    for (int i = 0; i < rowsOfData; i++){
        Point center = A + (((float) i) / (rowsOfData-1)) * (B - A) - (6.5*pitch/cv::norm(ortho))*ortho;
        char text[3];
        sprintf(text, "%02X", data[i]);
        Size textSize = getTextSize(text, fontFace, fontScale, thickness, &baseline);

        // center the text
        Point textOffset((-textSize.width)/2,
                         (textSize.height)/2);
        
        putText(src, text, center+textOffset, fontFace, fontScale, COLOR_GREY, thickness, /* lineType */8, /* bottomLeftOrigin */ false);
    }

    fontScale += 1;
    for (int i = 0; i < rowsOfData; i++){
        Point center = A + (((float) i) / (rowsOfData-1)) * (B - A) - (7.2*pitch/cv::norm(ortho))*ortho;
        char text[2];
        data[i] &= BIT_MASK;
        sprintf(text, "%c", data[i]);
        Size textSize = getTextSize(text, fontFace, fontScale, thickness, &baseline);

        // center the text
        Point textOffset((-textSize.width)/2,
                         (textSize.height)/2);
        
        putText(src, text, center+textOffset, fontFace, fontScale, COLOR_BLUE, thickness, /* lineType */8, /* bottomLeftOrigin */ false);
    }

    imshow(PROGRAM_NAME, src);
}

void output_rom_file(const char* rom_filename){

    /****************************************************
     * Determine start and end values so that we can    *
     * trim any 0x00 bytes that we may have in the head *
     * and tail of the tape scanned image.              *
     ****************************************************/

    int start=-1, end=-1;
    for (size_t i = 0; i < bits.size(); i++){
        if (data[i] != 0) {
            start = i;
            break;
        }
    }

    for (size_t i = bits.size() - 1; i >= 0; i--){
        if (data[i] != 0) {
            end = i;
            break;
        }
    }

    /***********************************************
     * Output the dumped data to a ROM image file. *
     ***********************************************/

    if (start != -1 && end != -1){
        FILE* fp = fopen(rom_filename, "wb");
        fwrite(&data[start], sizeof(unsigned char), end-start+1, fp);
        fclose(fp);
    }
}

int main(int argc, char** argv){
    namedWindow(PROGRAM_NAME, WINDOW_AUTOSIZE);

    if (argc < 4) {
        printf("\n\tusage: %s <punched_tape_image.png> <output.rom> <exchange_points>\n", argv[0]);
        printf("\n\t  exchange_points must be eigther 0 or 1");
        printf("\n\t  and it indicates whether or not the data in the tape image is flipped.\n");
        printf("\n\t  (yes... this should ideally be automatically detected in a future implementation!)\n\n");
        return -1;
    }

    if (argv[3][0] == '1') exchange_points = true;

    init_tape_scanner();

    /***************************************************
     * Load the image and preprocess it so that we      *
     * have the best possible results.                  *
     ****************************************************/

    Mat src;

    // Read the image
    src = imread(argv[1], 1);
    if (!src.data) return -1;

    process_frame(src, /* with_pauses */ true);

    output_rom_file(argv[2]);

    /************************************************************
     * Wait for the user to press any key to close the program. *
     ************************************************************/
    waitKey(0);    
    return 0;
}
