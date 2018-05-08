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


#include <portsfcpp.hpp>

namespace PortsfCpp {

namespace {

struct PortSfInitializer
{
	PortSfInitializer ()
	{
		psf_init();
	}

	~PortSfInitializer ()
	{
		psf_finish ();
	}

	static PortSfInitializer instance;
};

PortSfInitializer PortSfInitializer::instance;

} // anonymous

namespace Audio {

BaseStream::BaseStream (const int sfd, const PSF_PROPS& props)
: sfd_ (sfd)
, props_ (props)
{
}

BaseStream::~BaseStream ()
{
	psf_sndClose (sfd_);
}

psf_format BaseStream::getFormat () const
{
	return props_.format;
}

psf_stype BaseStream::getSampleType () const
{
	return props_.samptype;
}

psf_channelformat BaseStream::getChannelFormat () {
	return props_.chformat;
}

double BaseStream::getSampleRate () const {
	return props_.srate;
}

uint16 BaseStream::getNumChannels () const
{
	return props_.chans;
}

uint16 BaseStream::getPosition () const {
	return psf_sndTell (sfd_);
}

bool BaseStream::setPosition (const uint16 position)
{
	return psf_sndSeek (sfd_, position, PSF_SEEK_SET);
}

uint16 BaseStream::getNumFrames () const {
	return psf_sndSize (sfd_);
}

ReadStream::ReadStream (const int sfd, const PSF_PROPS& props)
: BaseStream (sfd, props)
{}

ReadStream* ReadStream::create (int sfd, const PSF_PROPS& props) {
	if (sfd == -1)
		return nullptr;

	return new ReadStream (sfd, props);
}

uint16 ReadStream::read (float* out, const uint16 numFrames)
{
	return psf_sndReadFloatFrames (sfd_, out, numFrames);
}

uint16 ReadStream::read (double* out, const uint16 numFrames)
{
	return psf_sndReadDoubleFrames (sfd_, out, numFrames);
}

WriteStream::WriteStream (const int sfd, const PSF_PROPS& props)
	: BaseStream (sfd, props)
{}

WriteStream* WriteStream::create (int sfd, const PSF_PROPS& props) {
	if (sfd == -1)
		return nullptr;

	return new WriteStream (sfd, props);
}

uint16 WriteStream::write (short* in, const uint16 numFrames)
{
	return psf_sndWriteShortFrames (sfd_, in, numFrames);
}

uint16 WriteStream::write (float* in, const uint16 numFrames)
{
	return psf_sndWriteFloatFrames (sfd_, in, numFrames);
}

uint16 WriteStream::write (double* in, const uint16 numFrames)
{
	return psf_sndWriteDoubleFrames (sfd_, in, numFrames);
}

PSF_PROPS getCDDAFormat () {
	PSF_PROPS props;
	props.srate = 44100;
	props.chans = 2;
	props.samptype = psf_stype::PSF_SAMP_16;
	props.format = psf_format::PSF_STDWAVE;
	props.chformat = psf_channelformat::MC_STEREO;

	return props;
}

} // namespace Audio

namespace FileSystem {

OpenParams::OpenParams (const std::string& path)
	: path_ (path)
{}

const std::string& OpenParams::getPath () const {
	return path_;
}

void OpenParams::mustRescale (const bool rescale) {
	rescale_ = rescale;
}

bool OpenParams::mustRescale () const {
	return rescale_;
}

Audio::ReadStream* open (const OpenParams& params) {
	PSF_PROPS props;
	auto sfd = psf_sndOpen (params.getPath ().data (), &props, params.mustRescale ());
	if (sfd == -1)
		return nullptr;

	return Audio::ReadStream::create (sfd, props);
}

CreateParams::CreateParams (const std::string& path, const PSF_PROPS& props)
	: path_ (path)
	, props_ (props)
{}

const std::string& CreateParams::getPath () const {
	return path_;
}

const PSF_PROPS& CreateParams::getProperties () const {
	return props_;
}

void CreateParams::mustSupportsClippingOfFloats (const bool clipFloats) {
	clipFloats_ = clipFloats;
}

bool CreateParams::mustSupportsClippingOfFloats () const {
	return clipFloats_;
}

void CreateParams::setMinimumHeader (const int minHeader) {
	minHeader_ = minHeader;
}

int CreateParams::getMinHeader () const {
	return minHeader_;
}

void CreateParams::setMode (const int mode) {
	mode_ = mode;
}

int CreateParams::getMode () const {
	return mode_;
}

Audio::WriteStream* create (const CreateParams& params) {
	auto sfd = psf_sndCreate (params.getPath ().data (), &params.getProperties (), params.mustSupportsClippingOfFloats (), params.getMinHeader (), params.getMode ());
	if (sfd == -1)
		return nullptr;

	return Audio::WriteStream::create (sfd, params.getProperties ());
}

} // namespace FileSystem


} // namespace PortsfCpp
