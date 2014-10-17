#! /usr/bin/env python

# example use of lib_dscapture: show jetized depth video stream
# Copyright (C) 2014, James Jolly (jamesjolly@gmail.com)
# See MIT-LICENSE.txt for legalese.

import os
import threading
import time

import numpy
import cv2

import lib_dscapture as dsc
import jetize as jet

global g_timer

c_UPDATE_INTERVAL = 0.10 # s
c_TICK_INTERVAL = 0.05 # s

def plot_event():
	im = dsc.get_frame()
	im_jet = jet.jetize(im)
	im_jet = cv2.resize(im_jet, (640, 480))
	cv2.imshow('view', im_jet)
	cv2.waitKey(1)

class timed_event(threading.Thread):

	def __init__(self, interval, event_handler):
		threading.Thread.__init__(self)
		self.setDaemon(True)
		self.interval = interval
		self.handler = event_handler

	def run(self):
		time.sleep(self.interval)
		self.handler()
		g_timer = timed_event(self.interval, self.handler)
		g_timer.start()

def main():
	g_timer = timed_event(c_UPDATE_INTERVAL, plot_event)
	g_timer.start()
	while True:
		time.sleep(c_TICK_INTERVAL)
		print round(time.time(), 1), dsc.at(), dsc.ts()

if __name__ == '__main__':

	g_timer = None
	dsc.start()	
	main()

