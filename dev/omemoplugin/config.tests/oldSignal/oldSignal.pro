CONFIG += c++11
SOURCES += main.cpp

unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += libsignal-protocol-c
}
