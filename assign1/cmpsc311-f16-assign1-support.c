/**
 * File		: cmpsc311-f16-assign1-support.c 
 * Description	: This is the implementation of the functions
 *		  in cmpsc311-f16-assign1-support.h
 *
 * Author	: Xuannan Su
 * Last Modified: 9/10/2016
 */

// Include Files
#include <stdio.h>
#include <math.h>

/*
 * Function	: float_display_array
 * Description	: Print out the array of floating point values
 *
 * Inputs	: array_length - the length of the array
 *		  float_array[] - the array itself
 * Outputs	: 0 if successful test
 */
int float_display_array (int array_length, float float_array[]) {
	
	printf("Floating point array:\n");
	
	//Print out each value
	for (int i=0; i<array_length; i++) {
		printf("float[%d] = %.3f\n", i, float_array[i]);
	}
	printf("\n");
	return 0;

}

/*
 * Function	: integer_display_array 
 * Description	: Print out the array of integer values
 *
 * Inputs	: array_length - the length of the array
 *		  int_array[] - the array itself
 * Outputs	: 0 if successful test
 */
int integer_display_array (int array_length, int int_array[]) {

	printf("Integer array:\n");
	
	//Print out each value
	for (int i=0; i<array_length; i++) {
		printf("integer[%d] = %d\n", i, int_array[i]);
	}
	printf("\n");
	return 0;

}

/*
 * Function	: float_evens
 * Description	: Count the number of even numbers in the floating point array
 *
 * Inputs	: float_array[] - the array itself
 *		  array_length - the length of the array
 * Outputs	: count - the number of even numbers in the array
 */
int float_evens(float float_array[], int array_length) {
        			
	int count = 0;
	for (int i=0; i<array_length; i++) {
		//check if even
		if ((int)float_array[i]%2 == 0)
			count++;
	}
	return count;

}

/*
 * Function	: integer_evens
 * Description	: Count the number of even numbers in the integer array
 *
 * Inputs	: int_array[] - the array itself
 *		  arrayy_length - the length of the array
 * Outputs	: count - the number of the even numbers in the array
 */
int integer_evens(int int_array[], int array_length) {

	int count = 0;
	for (int i=0; i<array_length; i++) {
		//check if even
		if (int_array[i]%2 == 0)
			count++;

	}
	return count;
	
}

/*
 * Function	: make_array
 * Description	: Make an exponentiated set of values
 *
 * Inputs	: arr[] - the array itself
 *		  range - the number of elements in the array
 *		  exp - the value to use in an exponent
 *		  md - the modulus value
 * Outputs	: 0 if successful test
 */
int make_array(int arr[], int range, int exp, int md) {
	
	//prinf out the header
	printf("Array (%d^i) mod %d\n", exp, md);
	
	//make the array
	for (int i=0; i<range; i++) {
		
		arr[i]=(long)pow(exp, i)%md;
		//print out each value
		printf("arr[%d] = %d\n", i, arr[i]);

	}
	printf("\n");

	return 0;

}

/*
 * Function	: most_value
 * Description	: Print out the values with the most occurences in array
 *
 * Inputs	: arr[] - the array itself
 *		  range - the number of elements in the array
 *		  maxval - the largest possible value in the array
 * Outputs	: 0 if successful test
 */
int most_values(int arr[], int range, int maxval) {

	int count[50]={ [0 ... 49] = 0};	//array to store each value's occurences
	int max_count=0;	//the max occurences time

	//count each value
	for (int i=0; i<range; i++) {
		count[arr[i]]++;
	}
	//determine the max occurences
	for (int i=0; i<maxval; i++) {
		if (count[i]>max_count){
			max_count = count[i];
		}
	}
	//print out the value
	for (int i=0; i<maxval; i++) {
		if (count[i]==max_count) {
			printf("Value %d is a most frequently occuring value (%d times).\n", i, count[i]);
		}
	}
	
	printf("\n");
	return 0;


}

/*
 * Function	: graph_functions
 * Description	: Print out the function cos(x)*mult1 and sin(x)*mult2
 *		  Print '.' for region between functions
 * Inputs	: mult1 - floating point number passed to the cos function
 *		  mult2 - floating point number passed to the sin function 
 * Outputs	: 0 if successful test
 */
int graph_functions(float mult1, float mult2) {

	char pixel[61][61];
	float y_axis_value[61];
	
	//Initialize the pixel
	for (int x=0; x<61; x++){
		for (int y=0; y<61; y++){
			pixel[x][y]=' ';
		}
	}
	
	//Initialize the y-axis value
	for (int i=0; i<61; i++) {
		
		y_axis_value[i] = (i-30)/10.0;

	}

	//Determine the x-axis
	for (int x=0; x<61;x++ ) {
		pixel[x][30]='-';
	}
		
	//Determine the y-axis
	for (int y=0; y<61;y++) {
		pixel[30][y]='|';
	}

	//Calculate the cos wave and sin wave points
	for (float x=-3.0; x<=3.0; x=x+0.1){

		//Calculate the array index for correspond x value
		int x_index = x*10+30.5;
		//Calculate y = cos(x) value for each x
		//Determine its position in two dimention array
		int cos_y = mult1*cos(x)*10+30.5;

		//check if it is out of the range
		if (cos_y>=0 && cos_y<=60)
			pixel[x_index][cos_y]='D';
		
		//Calculate y = sin(x) value for each x
		//Determine its position in two dimention array
		int sin_y = mult2*sin(x)*10+30.5;

		//check if it is out of the range
		if (sin_y>=0 && sin_y<=60)
			pixel[x_index][sin_y]='E';

		//Determine the area between cos and sin
		if (cos_y < sin_y) {
			for (int y=cos_y+1; y<sin_y; y++) {
				//check if it is out of the range
				if (y>=0 && y<=60)
					pixel[x_index][y]='.';
			}	
		} else {
			for (int y=sin_y+1; y<cos_y; y++) {
				//check if it is out of the range
				if (y>=0 && y<=60)
					pixel[x_index][y]='.';
			}
		}		
		
	}

	//Draw
	printf("             Graph: D = cos(x)*%.1f, E = sin(x)*%.1f\n", mult1, mult2);
	printf("     -------------------------------------------------------------\n");
	for (int y=60; y>-1; y--){
		printf("% .1f|", y_axis_value[y]);
		for (int x=0; x<61; x++){
			printf("%c", pixel[x][y]);
		}
		printf("|\n");
	}
	printf("\n    -3        -2        -1         0         1         2         3\n");
	printf("     0123456789012345678901234567890123456789012345678901234567890\n");

	return 0;

}
