#! /usr/local/bin/python

#
# Created on Thu Dec 30 2021
#
# Copyright (c) 2021 Cezar Menezes
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Contact: cezar.menezes@live.com
#
#

import math
import argparse
import numpy as np
import matplotlib.pyplot as plt

#
# Calculate an angle, given its sine an cosine values
#
def angle(i_sin,i_cos):    

    a = math.asin(max(min(i_sin,1.0),-1.0))
    return (math.pi-a) if (i_cos<0) else a

#
# Predict the Signal of 'theta_12' angle, given the sines and cosines
# 
def predict_theta_12_signal(sin_theta_12, cos_theta_12, sin_theta_13, cos_theta_13, sin_theta_1, cos_theta_1, debug=False):

    sin_theta_12_plus_theta_13  = sin_theta_12 * cos_theta_13 + cos_theta_12 * sin_theta_13
    sin_theta_12_minus_theta_13 = sin_theta_12 * cos_theta_13 - cos_theta_12 * sin_theta_13
    sin_theta_13_minus_theta_12 = sin_theta_13 * cos_theta_12 - cos_theta_13 * sin_theta_12
    sin_theta_13_plus_theta_1   = sin_theta_13 * cos_theta_1  + cos_theta_13 * sin_theta_1

    error = []
    error.append(abs(sin_theta_12_plus_theta_13  - sin_theta_1))
    error.append(abs(sin_theta_12_plus_theta_13  + sin_theta_1))
    error.append(abs(sin_theta_12_minus_theta_13 - sin_theta_1))
    error.append(abs(sin_theta_12_minus_theta_13 + sin_theta_1))    

    signal = [1.0,-1.0,1.0,-1.0] 

    index = error.index(min(error))

    if (debug):
        print("\nSignal Estimation (K):")
        print("\tIndex= {}  Min Error= {:.3e}  K= {}".format(index, error[index], signal[index]))

    return signal[index]

#
# Verify the "Triangularity" of a Triangle
#
def verify_single_triangularity(A,B,C):
    s = 0.5 * (A+B+C)
    return 0 if ( (A>s) or (B>s) or (C>s) ) else 1

#
# Verify the "Triangularity" of Triangles ST0-ST1-ST2, ST0-ST1-ST3 and ST1-ST2-ST3
#
def verify_all_triangularities(D01,D02,D03,D12,D13,D23,debug=False):
    code = 0x00

    if (not verify_single_triangularity(D01,D02,D12)):
        code = code | 0x01 
        if (debug):
            print("[!] Triangularity error in Triangle ST0-ST1-ST2")

    if (not verify_single_triangularity(D01,D03,D13)):
        code = code | 0x02
        if (debug):
            print("[!] Triangularity error in Triangle ST0-ST1-ST3")

    if (not verify_single_triangularity(D12,D13,D23)):
        code = code | 0x04
        if (debug):
            print("[!] Triangularity error in Triangle ST1-ST2-ST3")

    return code

#
# Calculate Sines and Cosines, given the trilateration distances
#
def calculate_sin_cos(D01,D02,D03,D12,D13,D23,debug=False):

    triangularity_error = verify_all_triangularities(D01,D02,D03,D12,D13,D23,debug)

    if ( triangularity_error ):
        # Calculate Bounded Cosines ( by Law of Cosines )
        cos_theta_12 = min(max((math.pow(D01,2.0) + math.pow(D12,2.0) - math.pow(D02,2.0)) / ( 2 * D01 * D12),-1.0),1.0)
        cos_theta_13 = min(max((math.pow(D01,2.0) + math.pow(D13,2.0) - math.pow(D03,2.0)) / ( 2 * D01 * D13),-1.0),1.0)
        cos_theta_1  = min(max((math.pow(D12,2.0) + math.pow(D13,2.0) - math.pow(D23,2.0)) / ( 2 * D12 * D13),-1.0),1.0)
        # Calculate Sines ( by Pythagoras )
        sin_theta_12 = math.sqrt( 1 - math.pow(cos_theta_12,2.0)) 
        sin_theta_13 = math.sqrt( 1 - math.pow(cos_theta_13,2.0)) 
        sin_theta_1  = math.sqrt( 1 - math.pow(cos_theta_1 ,2.0))
    else: 
        # Calculate Sines ( by Law of Sines ) 
        s12 = 0.5 * (D01 + D02 + D12)
        s13 = 0.5 * (D01 + D03 + D13)
        s1  = 0.5 * (D12 + D13 + D23)
        sin_theta_12 = ( 2.0 / (D01 * D12) ) * math.sqrt(s12 * (s12-D01) * (s12-D02) * (s12-D12))
        sin_theta_13 = ( 2.0 / (D01 * D13) ) * math.sqrt(s13 * (s13-D01) * (s13-D03) * (s13-D13))
        sin_theta_1 =  ( 2.0 / (D12 * D13) ) * math.sqrt(s1  * (s1-D12)  * (s1-D13)  * (s1-D23))
        # Calculate Cosines ( by Law of Cosines )
        cos_theta_12 = (math.pow(D01,2.0) + math.pow(D12,2.0) - math.pow(D02,2.0)) / ( 2 * D01 * D12)
        cos_theta_13 = (math.pow(D01,2.0) + math.pow(D13,2.0) - math.pow(D03,2.0)) / ( 2 * D01 * D13)
        cos_theta_1  = (math.pow(D12,2.0) + math.pow(D13,2.0) - math.pow(D23,2.0)) / ( 2 * D12 * D13)

    if (debug):
        if (triangularity_error):
            print("[!] Due to the Triangulary Error, some angles will be approximated.")

        print("\nCosines and Sines:")
        print("\tcos_theta_12= {:.3f}\tsin_theta_12= {:.3f}".format(cos_theta_12,sin_theta_12))
        print("\tcos_theta_13= {:.3f}\tsin_theta_13= {:.3f}".format(cos_theta_13,sin_theta_13))
        print("\tcos_theta_1=  {:.3f}\tsin_theta_1=  {:.3f}".format(cos_theta_1,sin_theta_1))    
        print("\nAngles:")
        print("\ttheta_12= {:.1f} degrees".format(angle(sin_theta_12,cos_theta_12)*180.0/math.pi))
        print("\ttheta_13= {:.1f} degrees".format(angle(sin_theta_13,cos_theta_13)*180.0/math.pi))
        print("\ttheta_1=  {:.1f} degrees".format(angle(sin_theta_1,cos_theta_1)*180.0/math.pi))


    return (sin_theta_12, cos_theta_12, sin_theta_13, cos_theta_13, sin_theta_1, cos_theta_1)

#
# Predict the X,Y coordinates, given the trilateration distances
#
def predict_xy(D01,D02,D03,D12,D13,D23,debug=False):

    (sin_theta_12, cos_theta_12, sin_theta_13, cos_theta_13, sin_theta_1, cos_theta_1) = calculate_sin_cos(D01,D02,D03,D12,D13,D23,debug)    
    K = predict_theta_12_signal(sin_theta_12, cos_theta_12, sin_theta_13, cos_theta_13, sin_theta_1, cos_theta_1, debug)
    return (cos_theta_12 * D01, K * sin_theta_12 * D01, 0.0, 0.0, D12, 0.0, D13 * cos_theta_1, D13 * sin_theta_1)

################
# MAIN FUNCTON #
################

if __name__ == "__main__":

    mode = None

    #
    # Read Input Arguments
    #
    parser = argparse.ArgumentParser(prog='angle')

    subparsers = parser.add_subparsers(help='sub-command help',dest='command')

    #
    # 'Coordinates' Command Arguments
    #
    parser_a = subparsers.add_parser('coordinates', help='positions help')
    parser_a.add_argument('-x0', type=float,  help='X0 coordinate')
    parser_a.add_argument('-y0', type=float,  help='Y0 coordinate')
    parser_a.add_argument('-x1', type=float,  help='X1 coordinate')
    parser_a.add_argument('-y1', type=float,  help='Y1 coordinate')
    parser_a.add_argument('-x2', type=float,  help='X2 coordinate')
    parser_a.add_argument('-y2', type=float,  help='Y2 coordinate')
    parser_a.add_argument('-x3', type=float,  help='X3 coordinate')
    parser_a.add_argument('-y3', type=float,  help='Y3 coordinate')

    #
    # 'Distances' Command Arguments
    #

    parser_b = subparsers.add_parser('distances', help='distances help')    
    parser_b.add_argument('-d01', type=float, help='D01 distance')
    parser_b.add_argument('-d02', type=float, help='D02 distance')
    parser_b.add_argument('-d03', type=float, help='D03 distance')
    parser_b.add_argument('-d12', type=float, help='D12 distance')
    parser_b.add_argument('-d13', type=float, help='D13 distance')
    parser_b.add_argument('-d23', type=float, help='D23 distance')

    args = vars(parser.parse_args())

    mode = args['command']

    if( mode == 'coordinates' ) :
        ####################
        # Coordinates Mode #
        ####################
        x0 = args['x0']
        y0 = args['y0']    
        x1 = args['x1']
        y1 = args['y1']
        x2 = args['x2']
        y2 = args['y2']
        x3 = args['x3']
        y3 = args['y3']
        if ( (x0 != None) and (y0 != None) and (x1 != None) and (y1 != None) and (x2 != None) and (y2 != None) and (x3 != None) and (y3 != None) ):
            print("\nSimulation Mode: {}".format(mode))        
            D01 = math.sqrt(math.pow(x1-x0,2.0) + math.pow(y1-y0,2.0))
            D02 = math.sqrt(math.pow(x2-x0,2.0) + math.pow(y2-y0,2.0))
            D03 = math.sqrt(math.pow(x3-x0,2.0) + math.pow(y3-y0,2.0))
            D12 = math.sqrt(math.pow(x2-x1,2.0) + math.pow(y2-y1,2.0))
            D13 = math.sqrt(math.pow(x3-x1,2.0) + math.pow(y3-y1,2.0))
            D23 = math.sqrt(math.pow(x3-x2,2.0) + math.pow(y3-y2,2.0))
            print("\td01={:.2f} d02={:.2f} d03={:.2f}".format(D01,D02,D03))
            print("\td12={:.2f} d13={:.2f} d23={:.2f}".format(D12,D13,D23))
        else :
            print("Missing Parameters")
            exit()

    elif( args['command'] == 'distances' ) :
        ####################
        # Distances Mode   #
        ####################
        D01 = args['d01']
        D02 = args['d02']
        D03 = args['d03']
        D12 = args['d12']
        D13 = args['d13']
        D23 = args['d23']

        if ( (D01 != None) and (D02 != None) and (D03 != None) and (D12 != None) and (D13 != None) and (D23 != None) ):
            print("\nSimulation Mode: {}".format(mode))                
        else :
            print("Missing Parameters")
            exit()
    else :
        print("Missing Command")
        exit()

    #
    # Calculate X,Y Predictions
    #

    (predict_x0, predict_y0, predict_x1, predict_y1, predict_x2, predict_y2, predict_x3, predict_y3) = predict_xy(D01,D02,D03,D12,D13,D23,debug=True)

    print("\nX,Y Coordinates Estimation:")

    print("\tST0 : [ {:.1f} , {:.1f} ]".format(predict_x0,predict_y0))

    print("\tST1 : [ {:.1f} , {:.1f} ]".format(predict_x1,predict_y1))

    print("\tST2 : [ {:.1f} , {:.1f} ]".format(predict_x2,predict_y2))

    print("\tST3 : [ {:.1f} , {:.1f} ]".format(predict_x3,predict_y3))

    #
    # Plot X,Y Predictions
    #

    ax = np.array([predict_x0, predict_x1,predict_x2,predict_x3])
    ay = np.array([predict_y0, predict_y1,predict_y2,predict_y3])
    labels = ['ST0','ST1','ST2','ST3']
    plt.scatter(ax, ay)
    plt.title("FTM Station Distribution")
    for k in range(0,4):
        plt.text(ax[k]+.05,ay[k]+.05,labels[k],fontsize=9)
    plt.savefig('figure.png')        
    plt.show()
   
