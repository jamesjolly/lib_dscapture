
import threading
import time

class repeat_event(threading.Thread):

	def __init__(self, interval, event_handler):
		threading.Thread.__init__(self)
		self.setDaemon(True)
		self.interval = interval
		self.handler = event_handler

	def run(self):
		time.sleep(self.interval)
		self.handler()
		self = repeat_event(self.interval, self.handler)
		self.start()

