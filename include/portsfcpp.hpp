/* Copyright (c) 2018, Armand Rochette

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

// https://github.com/ethz-asl/programming_guidelines/wiki/Cpp-Coding-Style-Guidelines

#ifndef __PORTSFCPP_H_INCLUDED
#define __PORTSFCPP_H_INCLUDED

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <portsf.h>

namespace PortsfCpp {

using int16 = int;
using uint16 = unsigned int;
	
namespace Audio {

PSF_PROPS getCDDAFormat ();

class BaseStream
{
public:
	BaseStream (const int sfd, const PSF_PROPS& props);
	virtual ~BaseStream ();

	psf_format getFormat () const;
	psf_stype getSampleType () const;
	psf_channelformat getChannelFormat ();
	
	double getSampleRate () const;
	uint16 getNumChannels () const;

	uint16 getNumFrames () const;
	uint16 getPosition () const;
	bool setPosition (const uint16 position);

protected:
	const int sfd_;

private:
	const PSF_PROPS props_;
};
	
class ReadStream : public BaseStream
{
public:
	static ReadStream* create (int sfd, const PSF_PROPS& props);

	uint16 read (float* out, const uint16 numFrames);
	uint16 read (double* out, const uint16 numFrames);
	
private:
	ReadStream (const int sfd, const PSF_PROPS& props);
	ReadStream () = delete;
	ReadStream (const ReadStream&) = delete;
};


class WriteStream : public BaseStream
{
public:
	static WriteStream* create (int sfd, const PSF_PROPS& props);

	uint16 write (short* in, const uint16 numFrames);
	uint16 write (float* in, const uint16 numFrames);
	uint16 write (double* in, const uint16 numFrames);

private:
	WriteStream (const int sfd, const PSF_PROPS& props);
	WriteStream () = delete;
	WriteStream (const ReadStream&) = delete;
};

} // namespace Audio

namespace FileSystem {
	
class OpenParams
{
public:
	OpenParams (const std::string& path);
	
	const std::string& getPath () const;
	void mustRescale (const bool rescale);
	bool mustRescale () const;
private:
	OpenParams () = delete;

	const std::string path_;
	bool rescale_ = false;
};

class CreateParams
{
public:
	CreateParams (const std::string& path, const PSF_PROPS& props = Audio::getCDDAFormat ());

	const std::string& getPath () const;
	const PSF_PROPS& getProperties () const;

	void mustSupportsClippingOfFloats (const bool clipFloats);
	bool mustSupportsClippingOfFloats () const;
	void setMinimumHeader (const int minHeader);
	int getMinHeader () const;
	void setMode (const int mode);
	int getMode () const;
private:
	CreateParams () = delete;

	const std::string path_;
	const PSF_PROPS props_;
	int clipFloats_ = 0;
	int minHeader_ = 0;
	int mode_ = 0;
};

template <typename InFloat, typename OutFloat>
using ProcessCallback = std::function<bool (const InFloat*, OutFloat*, const uint16)>;

Audio::ReadStream* open (const OpenParams& params);
Audio::WriteStream* create (const CreateParams& params);

template <typename InFloat, typename OutFloat>
bool openAndProcess (const OpenParams& oParams, const CreateParams& cParams, ProcessCallback<InFloat, OutFloat> processCall) {
	auto rStream = std::unique_ptr<Audio::ReadStream> (FileSystem::open (oParams));
	if (rStream == nullptr)
		return false;

	auto wStream = std::unique_ptr<Audio::WriteStream> (FileSystem::create (cParams));
	if (wStream == nullptr)
		return false;

	return openAndProcess (*rStream, *wStream, processCall);
}

template <typename InFloat, typename OutFloat>
bool openAndProcess (Audio::ReadStream& rStream, Audio::WriteStream& wStream, ProcessCallback<InFloat, OutFloat> processCall) {
	static const uint16 kBlockSize = 512;
	auto framesLeft = rStream.getNumFrames ();
	std::vector<InFloat> in (kBlockSize * rStream.getNumChannels ());
	std::vector<OutFloat> out (kBlockSize * wStream.getNumChannels ());

	while (framesLeft > 0) {
		const auto numFrames = std::min<int16> (framesLeft, kBlockSize);

		if (rStream.read (in.data (), numFrames) != numFrames)
			break;

		if (processCall (in.data (), out.data (), numFrames)) {
			if (wStream.write (out.data (), numFrames) != numFrames)
				break;
		}
		else
			break;

		framesLeft -= numFrames;
	}

	return framesLeft == 0;
}

} // namespace FileSystem
	
} // namespace PortsfCpp

#endif
