#include <boost/asio.hpp>
#include <iostream>

#include "icmp_header.hpp"
#include "ipv4_header.hpp"

namespace bip = boost::asio::ip;

class Pinger
{
public:
  Pinger(boost::asio::io_service &iIoService, const std::string &iHostname)
    : mEndpoint(*bip::icmp::resolver(iIoService).resolve(bip::icmp::resolver::query(bip::icmp::v4(), iHostname, "")))
    , mSocket(iIoService, bip::icmp::v4())
    , mTimer(iIoService)
    , mSequenceNumber(0)
    , mNumReplies(0)
    , mNumErrors(0)
  {
    startSend();
    startReceive();
  }

private:
  void startSend()
  {
    std::string body("\"Hello!\" from Asio ping.");

    // Create an ICMP header for an echo request.
    icmp_header echo_request;
    echo_request.type(icmp_header::echo_request);
    echo_request.code(0);
    echo_request.identifier(getIdentifier());
    echo_request.sequence_number(++mSequenceNumber);
    compute_checksum(echo_request, body.begin(), body.end());

    // Encode the request packet.
    boost::asio::streambuf request_buffer;
    std::ostream os(&request_buffer);
    os << echo_request << body;

    // Send the request.
    mTimeSent = boost::posix_time::microsec_clock::universal_time();
    mSocket.send_to(request_buffer.data(), mEndpoint);

    // Wait up to five seconds for a reply.
    mNumReplies = 0;
    mTimer.expires_at(mTimeSent + boost::posix_time::seconds(5));
    mTimer.async_wait(std::bind(&Pinger::handleTimeout, this));
  }

  void handleTimeout()
  {
    if (mNumReplies == 0)
    {
      if (++mNumErrors >= 5)
      {
        //std::cout << "handle disconnect" << std::endl;
        exit(1);
      }
      //std::cout << mNumErrors << std::endl;
    }

    // Requests must be sent no less than one second apart.
    mTimer.expires_at(mTimeSent + boost::posix_time::seconds(1));
    mTimer.async_wait(std::bind(&Pinger::startSend, this));
  }

  void startReceive()
  {
    // Discard any data already in the buffer.
    mReplyBuffer.consume(mReplyBuffer.size());

    // Wait for a reply. We prepare the buffer to receive up to 64KB.
    mSocket.async_receive(mReplyBuffer.prepare(65536),
      std::bind(&Pinger::handleReceive, this, std::placeholders::_2));
  }

  void handleReceive(std::size_t length)
  {
    // The actual number of bytes received is committed to the buffer so that we
    // can extract it using a std::istream object.
    mReplyBuffer.commit(length);

    // Decode the reply packet.
    std::istream is(&mReplyBuffer);
    ipv4_header ipv4_hdr;
    icmp_header icmp_hdr;
    is >> ipv4_hdr >> icmp_hdr;

    // We can receive all ICMP packets received by the host, so we need to
    // filter out only the echo replies that match the our identifier and
    // expected sequence number.
    if (is && icmp_hdr.type() == icmp_header::echo_reply
      && icmp_hdr.identifier() == getIdentifier()
      && icmp_hdr.sequence_number() == mSequenceNumber)
    {
      // If this is the first reply, interrupt the five second timeout.
      if (mNumReplies++ == 0)
      {
        mTimer.cancel();
      }

      mNumErrors = 0;
      //std::cout << mNumErrors << std::endl;
    }

    startReceive();
  }

  static unsigned short getIdentifier()
  {
#if defined(BOOST_WINDOWS)
    return static_cast<unsigned short>(::GetCurrentProcessId());
#else
    return static_cast<unsigned short>(::getpid());
#endif
  }

  bip::icmp::endpoint mEndpoint;
  bip::icmp::socket mSocket;

  boost::asio::deadline_timer mTimer;
  unsigned short mSequenceNumber;
  boost::posix_time::ptime mTimeSent;
  boost::asio::streambuf mReplyBuffer;
  std::size_t mNumReplies;
  size_t mNumErrors;
};

int main(int argc, char *argv[])
{
  try
  {
    boost::asio::io_service wIoService;
    Pinger wPinger(wIoService, argv[1]);
    wIoService.run();
  }
  catch (std::exception &iException)
  {
    std::cerr << iException.what() << std::endl;
    exit(2);
  }
  return 0;
}
