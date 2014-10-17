#! /usr/bin/env python

# example use of lib_dscapture: show jetized depth video stream
# Copyright (C) 2014, James Jolly (jamesjolly@gmail.com)
# See MIT-LICENSE.txt for legalese.

import time

import numpy
import cv2

import lib_dscapture as dsc
from jetize import jet
from timed_event import repeat_event

c_UPDATE_INTERVAL = 0.10 # s
c_TICK_INTERVAL = 0.05 # s

c_DEPTH_FRAMERATE = 30 # Hz
c_CAMERA_MODE = 0 # {CLOSE_MODE = 0, LONG_RANGE = 1} 

def plot_event():
	dim = dsc.get_dframe()
	dim_jet_resized = cv2.resize(jet(dim), (640, 480))
	cv2.imshow('dview', dim_jet_resized)
	cv2.waitKey(1)

def main():
	timer = repeat_event(c_UPDATE_INTERVAL, plot_event)
	timer.start()
	while True:
		time.sleep(c_TICK_INTERVAL)
		print round(time.time(), 1), dsc.last_dtime(), dsc.last_dframe()

if __name__ == '__main__':

	dsc.start(c_DEPTH_FRAMERATE, c_CAMERA_MODE)	
	main()

