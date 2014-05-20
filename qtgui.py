# -*- coding: utf-8 -*-
#!/usr/bin/env python

from PyQt4 import QtGui, QtCore
from ctypes import *
import threading
import time
import sys
from clientlib import *

class MainWindow(QtGui.QWidget):
	def __init__(self, parent=None):
		QtGui.QMainWindow.__init__(self, parent)
		self.setWindowTitle("chatDemo")
		self.setFixedSize(250, 600)

		self.table = QtGui.QTableWidget(20, 1)
		self.table.setHorizontalHeaderLabels([u'online      '])
		self.table.verticalHeader().setVisible(False)
		self.table.setEditTriggers(QtGui.QTableWidget.NoEditTriggers)
		self.table.setSelectionBehavior(QtGui.QTableWidget.SelectRows)
		self.table.setSelectionMode(QtGui.QTableWidget.SingleSelection)
		self.table.setAlternatingRowColors(True)

		self.table.cellDoubleClicked.connect(self.chating)

		button = QtGui.QPushButton(u'聊天室', parent=self)

		button.clicked.connect(self.chatroom)

		layout = QtGui.QVBoxLayout()
		layout.addWidget(self.table)
		layout.addWidget(button)

		self.setLayout(layout)

	def chating(self):
		uid = self.table.currentItem().text()
		dialog = chat(self, uid)
		dialog.exec_()

	def chatroom(self):
		dialog = chat()
		dialog.exec_()


class chat(QtGui.QDialog, threading.Thread):
	def __init__(self, parent=None, uid="Noname"):
		QtGui.QDialog.__init__(self, parent)
		self.setWindowTitle(uid)
		self.resize(402, 392)
		self.uid = uid

		grid = QtGui.QGridLayout()

		self.outText = QtGui.QTextEdit(parent=self)
		grid.addWidget(self.outText, 0, 1, 1, 1)

		self.inText = QtGui.QTextEdit(self)
		grid.addWidget(self.inText, 1, 1, 1, 1)
		self.inText.setText("god")

		# 创建ButtonBox，用户确定和取消
		self.send = QtGui.QPushButton(u'发送', self)
		self.close = QtGui.QPushButton(u'关闭', self)
		self.send.clicked.connect(self.Send)
		self.connect(self.close,QtCore.SIGNAL("clicked()"),self, QtCore.SLOT("close()"))
		layout = QtGui.QVBoxLayout()
		layout.addLayout(grid)
		spacerItem = QtGui.QSpacerItem(20, 48, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
		layout.addItem(spacerItem)

		layout.addWidget(self.send)
		layout.addWidget(self.close)

		self.setLayout(layout)

	def Send(self):
		print 'Send'
		lib.sendMSG(str(self.uid) , str(self.inText.toPlainText()))
		return

class Sign(QtGui.QDialog):
	def __init__(self, parent=None):
		QtGui.QDialog.__init__(self, parent)
		self.setWindowTitle(u"注册")
		self.resize(240, 200)

		# 表格布局，用来布局QLabel和QLineEdit及QSpinBox
		grid = QtGui.QGridLayout()

		grid.addWidget(QtGui.QLabel(u'用户名', parent=self), 0, 0, 1, 1)

		self.leName = QtGui.QLineEdit(parent=self)
		grid.addWidget(self.leName, 0, 1, 1, 1)

		grid.addWidget(QtGui.QLabel(u'密码', parent=self), 1, 0, 1, 1)

		self.lePassword = QtGui.QLineEdit(parent=self)
		self.lePassword.setEchoMode(QtGui.QLineEdit.Password)
		grid.addWidget(self.lePassword, 1, 1, 1, 1)

		buttonBox = QtGui.QDialogButtonBox(parent=self)
		buttonBox.setOrientation(QtCore.Qt.Horizontal)
		buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
		buttonBox.accepted.connect(self.accept)
		buttonBox.rejected.connect(self.reject)

		layout = QtGui.QVBoxLayout()

		layout.addLayout(grid)

		spacerItem = QtGui.QSpacerItem(20, 48, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
		layout.addItem(spacerItem)

		layout.addWidget(buttonBox)

		self.setLayout(layout)


class LoginDialog(QtGui.QDialog):
	def __init__(self, parent=None):
		QtGui.QDialog.__init__(self, parent)
		self.setWindowTitle(u'登录')
		self.resize(300, 150)

		self.leName = QtGui.QLineEdit(self)
		self.leName.setPlaceholderText(u'用户名')

		self.lePassword = QtGui.QLineEdit(self)
		self.lePassword.setEchoMode(QtGui.QLineEdit.Password)
		self.lePassword.setPlaceholderText(u'密码')

		self.pbLogin = QtGui.QPushButton(u'登录', self)
		self.pbSign = QtGui.QPushButton(u'注册', self)

		self.pbLogin.clicked.connect(self.login)
		self.pbSign.clicked.connect(self.sign)

		layout = QtGui.QVBoxLayout()
		layout.addWidget(self.leName)
		layout.addWidget(self.lePassword)

		spacerItem = QtGui.QSpacerItem(20, 48, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
		layout.addItem(spacerItem)

		buttonLayout = QtGui.QHBoxLayout()
		spancerItem2 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
		buttonLayout.addItem(spancerItem2)
		buttonLayout.addWidget(self.pbLogin)
		buttonLayout.addWidget(self.pbSign)

		layout.addLayout(buttonLayout)

		self.setLayout(layout)

	def login(self):
		print 'login'
		uid = str(self.leName.text())
		lib.login(uid)
		threadloop.start()
		rl.start()
		self.accept()
#		if self.leName.text() == '' and self.lePassword.text() == '':
#			self.accept()
#		else:
#			QtGui.QMessageBox.critical(self, u'错误', u'用户名密码不匹配')
#
	def sign(self):
		print "Sign"
		dialog = Sign(parent=self)
		if dialog.exec_():
			print "Sign sucess"
			dialog.destroy()
		else:
			return False

class loop(threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)
	def run(self):
		lib.main_loop()

class refreshlist(threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)
	def run(self):
		while True:
			t = lib.getUsers
			t.restype = c_char_p
			users = lib.getUsers()
			userlist = users.split(':')
			t = lib.haveMSG
			t.restype = c_char_p
			have = lib.haveMSG()
			print have
			h = have.split(':')
			i = 0
			for value in userlist:
				newItem = QtGui.QTableWidgetItem(value)
				if len(have)!=0:
					for pos in h:
						if i == int(pos):
							newItem.setBackgroudColor(QtQgui.QColor(0,255,0))
				mainWindow.table.setItem(i, 0, newItem)
				i = i+1
			time.sleep(1)


def init():
	lib.init()
	return True

def login():
	dialog = LoginDialog()
	if dialog.exec_():
		return True
	else:
		return False

threadloop = loop()
threadloop.setDaemon(True)
rl = refreshlist()
rl.setDaemon(True)
app = QtGui.QApplication(sys.argv)
mainWindow = MainWindow()

if __name__ == '__main__':
	init()
	if login():
		mainWindow.show()
		sys.exit(app.exec_())
