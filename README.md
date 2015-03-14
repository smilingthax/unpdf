A C++ library to access PDF-files at the object level.
Functionality like "pdfselect" or even an "Acrobat Pro"-clone can then be implemented on top of it - the PDF-Reference will be your friend :-)

Have a look at [CurrentStatus](https://github.com/smilingthax/unpdf/wiki/CurrentStatus). 

This project also includes two independently usable C libraries: 
 * g4coder - a G3/G4 Fax En-/Decoder
 * lzwcoder - a (TIFF) LZW En-/Decoder
 
The wiki contains [some notes](https://github.com/smilingthax/unpdf/wiki/g4lzw).

Another somehow related project is libfontembed, which aims to encapsulate all the details required to embed TTF or OTF(CFF) (for now; also subsetting only for TTF) into PDF files (some PS support is already there). It is released under the MIT License and available via the Openprinting/Linuxfoundation bazaar repository, as its currenty primary user is texttopdf from cups-filters:  [View Bazaar Repository](http://bzr.linuxfoundation.org/loggerhead/openprinting/cups-filters/files/head:/filter/fontembed/)
