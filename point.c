#include "point.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void init_point(struct Point *p)
{
  for(int i=0; i < 2; i++)
  {
    p->min[i] = p->max[i] = p->center[i] = -1;
  }
  p->size = 0;
}

void add_line(struct Point *p, int left, int right)
{
  if(left == right)
  {
    add_point(p, left);
  }
  else if(p->size == 0)
  {
    p->min[0] = XVAL(left);
    p->max[0] = XVAL(right);
    p->min[1] = p->max[1] = YVAL(left); // should be the same column
    p->size += right - left;
  }
  else
  {
    if(XVAL(left) < p->min[0]) p->min[0] = XVAL(left);
    if(XVAL(right) > p->max[0]) p->max[0] = XVAL(right);
    p->max[1] = YVAL(left); // should just be one line and should be greater than last one
    p->size += right - left;
  }
}

void add_point(struct Point *p, int index)
{
  if(p->size == 0)
  {
    p->min[0] = p->max[0] = XVAL(index);
    p->min[1] = p->max[1] = YVAL(index); // should be the same column
  }
  else
  {
    if(XVAL(index) < p->min[0]) p->min[0] = XVAL(index);
    if(XVAL(index) > p->max[0]) p->max[0] = XVAL(index);
    p->max[1] = YVAL(index); // should just be one line and should be greater than last one
  }
  p->size++;
}

int* get_center(struct Point *p)
{
  for(int i=0; i < 2; i++)
    p->center[i] = p->min[i] + ((p->max[i] - p->min[i])/2);
  return p->center;
}

int test_point(struct Point *p, int index)
{
  if(p->size == 0) // new point
    return 1;
  if(YVAL(index) > (p->max[1]+1)) // should never be a break - done with point
    return -1;
  if(XVAL(index) < (p->min[0]-THRESHOLD) || XVAL(index) > (p->max[0]+THRESHOLD))
    return 0; // don't add to this point, but keep active

  return 1; // should be good, add to this point
}

int test_shape(struct Point *p)
{
  int width, height, skew;

  if(p->size < MIN_SIZE)  
    return -1;
  if(p->size > MAX_SIZE)
    return -2;

  width = p->max[0] - p->min[0];
  height = p->max[1] - p->min[1];
  skew = height - width;

  if(skew > MAX_SKEW || skew < MIN_SKEW)
    return -3;

  return 1;
}

int in_points(int (*points)[2], int length, int *center) {
  for(int i = 0; i < length; i++) {
    if(center[0] == points[i][0] && center[1] == points[i][1])
      return 1;
  }
  return 0;
}

int point_finder(int (*center_points)[2], int length)
{
  // point finder variables
  int left, right;
  const int threshold_C = 10;
  struct Point points[length];
  const int unused_C = 0;
  const int active_C = 1;
  const int consemated_C = 2;
  const int bad_C = -1;
  int point_status[length];
  int left_point = 0;
  int right_point = -1;
  int num_centers = 0;
  unsigned char working_line[WIDTH];

  // init the points
  for(int i = 0; i < length; i++) {
    init_point(&points[i]);
  }

  // point finder algorithm
  for(int y=0; y<HEIGHT; y++)
  {
    left = right = -1; // new line
    read_filtered_line(working_line);
    for(int x=0; x<WIDTH; x++) {
      if(*(working_line+x) > threshold_C)
      {
        if(left == -1) // new line of high values
        {
          left = right = y*WIDTH + x;
        }
        else
        {
          right = y*WIDTH + x;
        }
      } else if(left != -1) {
        bool found = false;
        for(int j=0; j<=right_point;j++)
        {
          if(point_status[j] == active_C)
          {
            int test = test_point(&points[j], left);
            if(test == 1)
            {
              add_line(&points[j],left,right);
              found = true;
              break;
            } else if (test == -1) {
              // point should be consemated - we went past this point
              int shape = test_shape(&points[j]); // test the shape
              if(shape == 1) {
                get_center(&points[j]); // calculates the center points
                printf("Good Shape (%d,%d)\n", points[j].center[0], points[j].center[1]);
                if(num_centers == length) {
                  printf("Out of memory in center_points\n");
                  continue;
                }
                if(!in_points(center_points,length,points[j].center)) {
                  center_points[num_centers][0] = points[j].center[0];
                  center_points[num_centers][1] = points[j].center[1];
                  num_centers++;
                }
              }
              // free the point
              point_status[j] = unused_C;
              init_point(&points[j]);
            }
          }
        }
        if(!found)
        {
          // reuse unused points
          int found_unused = 0;
          for(int j = 0; j <= right_point; j++) {
            if(point_status[j] == unused_C) {
              found_unused = 1;
              point_status[j] = active_C;
              add_line(&points[j], left, right);
            }
          }

          if(right_point == length)
            printf("Need more memory!\n");

          if(found_unused == 0 && right_point < length) { // get new point (bounded, safe)
            right_point++;
            point_status[right_point] = active_C;
            add_line(&points[right_point], left, right);
          }

        }
        left = -1;
      }
    }
  }

#ifdef DEBUG_FILE
  FILE *f = fopen("working.txt", "w");
  if (f == NULL)
  {
    printf("Error opening file!\n");
    exit(1);
  }

  reset_address();

  fprintf(f, "/* green values only, 640 x 480 pix */ \n\n");
  for(int i=1; i < (NUM_PIXELS); i++)
  {
    if(XVAL(i) == 0)
      read_filtered_line(working_line);
    bool center = false;
    if(i != 0 && (i-1)%(WIDTH) == 0)
      fprintf(f, "\n");
    for(int j = 0; j < num_centers; j++)
    {
      if(XVAL(i) == center_points[j][0] && YVAL(i) == center_points[j][1])
      {
        fprintf(f, "XXX,");
        center = true;
        break;
      }
    }
    if(!center)
      fprintf(f,"%3d,",working_line[XVAL(i)]);
    center = false;
  }

  fclose(f);
#endif
  
  printf("Right Point: %d\n", right_point);
  return num_centers;
}

int sort_by_col(int (*center_points)[2], int num_points, int* col_idx, int col_idx_size)
{
  int working_array[num_points][2]; // array for copying data points
  int column_number[num_points];    // the column number of each point
  int column_max[30];         // the xvalue of the point that's lowest in the column (allong y axis) maximum (+threshold)
  int column_min[30];         // the min x value, -1 once copied
  int num_col = 0;          // the number of columns found
  int const done_C = -1000;     // done with this column
  int point_count = 0;          // counter for adding values back into the array
  int col_idx_count = 0;        // counter for adding values to col_idx

  column_number[0] = num_col; // first element
  column_min[0] = center_points[0][0] - COL_THRESHOLD;
  column_max[0] = center_points[0][0] + COL_THRESHOLD;
  num_col++;
  working_array[0][0] = center_points[0][0];
  working_array[0][1] = center_points[0][1];

  for(int i = 1; i < num_points; i++) // i - point number
  {
    bool column_found = false;
    for(int j = 0; j < num_col; j++) // j - column number
    {
      if(center_points[i][0] <= column_max[j] && center_points[i][0] >= column_min[j])
      {
        // in this column
        column_number[i] = j;
        column_min[j] = center_points[i][0] - COL_THRESHOLD;
        column_max[j] = center_points[i][0] + COL_THRESHOLD;
        column_found = true;
      }
    }
    if(!column_found)
    {
      column_number[i] = num_col;
      column_min[num_col] = center_points[i][0] - COL_THRESHOLD;
      column_max[num_col] = center_points[i][0] + COL_THRESHOLD;
      num_col++;
    }
    column_found = false;

    working_array[i][0] = center_points[i][0]; // copy into working array 
    working_array[i][1] = center_points[i][1];
  }

  // sort the columns and the array
  for(int i = 0; i < num_col; i++) // i - column number
  {
    int lowest = -1;
    for(int j = 0; j < num_col; j++) // j = column number
    {
      if(lowest == -1 && column_min[j] != done_C)
      {
        lowest = j;
      }
      else if(column_max[lowest] > column_max[j] && column_min[j] != done_C)
      {
        lowest = j;
      }
    }
    column_min[lowest] = done_C;
    for(int j = 0; j < num_points; j++) // j - point number
    {
      if(column_number[j] == lowest)
      {
        center_points[point_count][0] = working_array[j][0];
        center_points[point_count][1] = working_array[j][1];
        point_count++;
      }
    }
    if(col_idx_count == 0)
      col_idx[col_idx_count++] = 0;
    if(col_idx_count < col_idx_size) // protect the array (shoud never be false)
      col_idx[col_idx_count++] = point_count; // store the values of where the columns start
  } 

  return num_col;
}

int sort_by_row(int (*center_points)[2], const int num_points, int* row_idx, int row_idx_size)
{
  int num_row = 0;
  int working_array_x[num_points]; // array for copying data points
  int working_array_y[num_points]; // array for copying data points
  int min = center_points[0][0];
  int max = min;

  row_idx[0] = 0; // first row always starts at first element

  // sort by row
  // copy into dummy arrays and find max and min x values
  for(int i = 0; i < num_points; i++) {
    working_array_x[i] = center_points[i][0];
    working_array_y[i] = center_points[i][1];
    if(working_array_x[i] < min) {
      min = working_array_x[i];
    }
    if(working_array_x[i] > max) {
      max = working_array_x[i];
    }
  }
  int idx = 0;

  for(int val = min; val <= max; val++) {
    for(int i = 0; i < num_points; i++) {
      if(working_array_x[i] == val) {
        center_points[idx][0] = working_array_x[i];
        center_points[idx][1] = working_array_y[i];
        idx++;
      }
    }
  }

  for(int i = 1; i < num_points; i++) // i - point number
  {
    if(abs(center_points[i][0] - center_points[i-1][0]) > ROW_THRESHOLD) { 
      // new row
      num_row++;
      //printf("%d, %d, %d\r\n", center_points[i][0], center_points[i-1][0], abs(center_points[i][0] - center_points[i-1][0]));
      row_idx[num_row] = i;
    }
  }

  return num_row;
}


