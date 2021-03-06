/*

Osqoop, an open source software oscilloscope.
Copyright (C) 2006 Stephane Magnenat <stephane at magnenat dot net>
Laboratory of Digital Systems http://www.eig.ch/labsynum.htm
Engineering School of Geneva http://www.eig.ch

See AUTHORS for more details about other contributors.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __USB_DEVICE_H
#define __USB_DEVICE_H

#include <QString>

//! Light interface for USB device. Used to abstract the OS specific calls
class USBDevice
{
public:
	//! Virtual destructor, do nothing
	virtual ~USBDevice() { }
	//! Open the interface
	virtual bool open(const QString &firmwareFilename) = 0;
	//! Close the interface
	virtual bool close(void) = 0;
	//! Set the interface number to setting alternateSetting
	virtual bool setInterface(unsigned number, unsigned alternateSetting) = 0;
	//! Read size bytes on pipeNum into buffer using bulk transfer
	virtual unsigned bulkRead(unsigned pipeNum, char *buffer, size_t size) = 0;
	//! Write size bytes on pipeNum from buffer using bulk transfer
	virtual unsigned bulkWrite(unsigned pipeNum, const char *buffer, size_t size) = 0;
	//! Get a handle for the USB Device
	virtual bool GetHandle(void) = 0;
	//! Set if we read in overlapped mode
	virtual void setOverlapped(bool Value) = 0;
};

#endif
