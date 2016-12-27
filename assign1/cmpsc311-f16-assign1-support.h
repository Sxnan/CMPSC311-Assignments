#ifndef CMPSC311_A1SUPPORT_INCLUDED
#define CMPSC311_A1SUPPORT_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File          : cmpsc311-f16-assign1-support.h
//  Description   : This is a set of general-purpose utility functions we use
//                  for the 311 homework assignment #1.
//
//  Author   : ????
//  Created  : ????

//
// Functional Prototypes

int float_display_array(int array_length, float float_array[]);
	// This function prints out the array of floating point values

int integer_display_array(int array_length, int int_array[]);
	// This function prints out the array of integer values

int float_evens(float float_array[], int array_length);
	// Return the number of even numbers in the array (float version)

int integer_evens(int int_array[], int array_length);
	// Return the number of even numbers in the array (int version)

int make_array(int arr[], int range, int exp, int md);
	// Make an exponentiated set of values

int most_values(int arr[], int range, int maxval);
	// Print out the values with the most occurences in array

int graph_functions(float mult1, float mult2);
	// Print out the functions cos(x)+d1 and tan(x)*e1
	// Honors: print '+' for region bettween functions

#endif // CMPSC311_A1SUPPORT_INCLUDED
