#include <fc/network/http/connection.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/io/sstream.hpp>
#include <fc/io/iostream.hpp>
#include <fc/exception/exception.hpp>
#include <fc/network/ip.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/stdio.hpp>


class fc::http::connection::impl 
{
  public:
   fc::tcp_socket sock;
   fc::ip::endpoint ep;
   impl() {
   }

   int read_until( char* buffer, char* end, char c = '\n' ) {
      char* p = buffer;
     // try {
          while( p < end && 1 == sock.readsome(p,1) ) {
            if( *p == c ) {
              *p = '\0';
              return (p - buffer)-1;
            }
            ++p;
          }
     // } catch ( ... ) {
     //   elog("%s", fc::current_exception().diagnostic_information().c_str() );
        //elog( "%s", fc::except_str().c_str() );
     // }
      return (p-buffer);
   }

   fc::http::reply parse_reply() {
      fc::http::reply rep;
      try {
        fc::vector<char> line(1024*8);
        int s = read_until( line.data(), line.data()+line.size(), ' ' ); // HTTP/1.1
        s = read_until( line.data(), line.data()+line.size(), ' ' ); // CODE
        rep.status = static_cast<int>(to_int64(fc::string(line.data())));
        s = read_until( line.data(), line.data()+line.size(), '\n' ); // DESCRIPTION
        
        while( (s = read_until( line.data(), line.data()+line.size(), '\n' )) > 1 ) {
          fc::http::header h;
          char* end = line.data();
          while( *end != ':' )++end;
          h.key = fc::string(line.data(),end);
          ++end; // skip ':'
          ++end; // skip space
          char* skey = end;
          while( *end != '\r' ) ++end;
          h.val = fc::string(skey,end);
          rep.headers.push_back(h);
          if( h.key == "Content-Length" ) {
             rep.body.resize( static_cast<size_t>(to_uint64( fc::string(h.val) ) ));
          }
        }
        if( rep.body.size() ) {
          sock.read( rep.body.data(), rep.body.size() );
        }
        return rep;
      } catch ( fc::exception& e ) {
        elog( "${exception}", ("exception",e.to_detail_string() ) );
        sock.close();
        rep.status = http::reply::InternalServerError;
        return rep;
      } 
   }
};



namespace fc { namespace http {

         connection::connection()
         :my( new connection::impl() ){}
         connection::~connection(){}


// used for clients
void       connection::connect_to( const fc::ip::endpoint& ep ) {
  my->sock.close();
  my->sock.connect_to( my->ep = ep );
}

http::reply connection::request( const fc::string& method, 
                                const fc::string& url, 
                                const fc::string& body, const headers& he ) {
	
  if( !my->sock.is_open() ) {
    wlog( "Re-open socket!" );
    my->sock.connect_to( my->ep );
  }
  try {
      fc::stringstream req;
      req << method <<" "<<url<<" HTTP/1.1\r\n";
      req << "Host: localhost\r\n";
      req << "Content-Type: application/json\r\n";
      for( auto i = he.begin(); i != he.end(); ++i )
      {
          req << i->key <<": " << i->val<<"\r\n";
      }
      if( body.size() ) req << "Content-Length: "<< body.size() << "\r\n";
      req << "\r\n"; 
      fc::string head = req.str();

      my->sock.write( head.c_str(), head.size() );
    //  fc::cerr.write( head.c_str() );

      if( body.size() )  {
          my->sock.write( body.c_str(), body.size() );
    //      fc::cerr.write( body.c_str() );
      }
    //  fc::cerr.flush();

      return my->parse_reply();
  } catch ( ... ) {
      my->sock.close();
      FC_THROW_EXCEPTION( exception, "Error Sending HTTP Request" ); // TODO: provide more info
   //  return http::reply( http::reply::InternalServerError ); // TODO: replace with connection error
  }
}

// used for servers
fc::tcp_socket& connection::get_socket()const {
  return my->sock;
}

http::request    connection::read_request()const {
  http::request req;
  fc::vector<char> line(1024*8);
  int s = my->read_until( line.data(), line.data()+line.size(), ' ' ); // METHOD
  req.method = line.data();
  s = my->read_until( line.data(), line.data()+line.size(), ' ' ); // PATH
  req.path = line.data();
  s = my->read_until( line.data(), line.data()+line.size(), '\n' ); // HTTP/1.0
  
  while( (s = my->read_until( line.data(), line.data()+line.size(), '\n' )) > 1 ) {
    fc::http::header h;
    char* end = line.data();
    while( *end != ':' )++end;
    h.key = fc::string(line.data(),end);
    ++end; // skip ':'
    ++end; // skip space
    char* skey = end;
    while( *end != '\r' ) ++end;
    h.val = fc::string(skey,end);
    req.headers.push_back(h);
    if( h.key == "Content-Length" ) {
       req.body.resize( static_cast<size_t>(to_uint64( fc::string(h.val) ) ));
    }
    if( h.key == "Host" ) {
       req.domain = h.val;
    }
  }
  // TODO: some common servers won't give a Content-Length, they'll use 
  // Transfer-Encoding: chunked.  handle that here.

  if( req.body.size() ) {
    my->sock.read( req.body.data(), req.body.size() );
  }
  return req;
}

fc::string request::get_header( const fc::string& key )const {
  for( auto itr = headers.begin(); itr != headers.end(); ++itr ) {
    if( itr->key == key ) { return itr->val; } 
  }
  return fc::string();
}
fc::vector<header> parse_urlencoded_params( const fc::string& f ) {
  int num_args = 0;
  for( size_t i = 0; i < f.size(); ++i ) {
    if( f[i] == '=' ) ++num_args;
  }
  fc::vector<header> h(num_args);
  int arg = 0;
  for( size_t i = 0; i < f.size(); ++i ) {
    while( f[i] != '=' && i < f.size() ) {
      if( f[i] == '%' ) { 
        h[arg].key += char((fc::from_hex(f[i+1]) << 4) | fc::from_hex(f[i+2]));
        i += 3;
      } else {
          h[arg].key += f[i];
          ++i;
      }
    }
    ++i;
    while( i < f.size() && f[i] != '&' ) {
      if( f[i] == '%' ) { 
        h[arg].val += char((fc::from_hex(f[i+1]) << 4) | fc::from_hex(f[i+2]));
        i += 3;
      } else {
        h[arg].val += f[i] == '+' ? ' ' : f[i];
        ++i;
      }
    }
    ++arg;
  }
  return h;
}

} } // fc::http