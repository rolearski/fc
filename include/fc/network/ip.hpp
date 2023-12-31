#pragma once
#include <fc/crypto/sha1.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/crypto/city.hpp>
#include <fc/reflect/reflect.hpp>

namespace fc {

  namespace ip {
    class address {
      public:
        explicit address( uint32_t ip = 0 );
        explicit address( const std::string& s );

        address& operator=( const std::string& s );
        address& operator=( uint32_t ip );
        explicit operator std::string()const;
        // Note: there are quite a few places where it is used implicitly, so we haven't marked it as explicit so far.
        operator uint32_t()const;

        friend bool operator==( const address& a, const address& b );
        friend bool operator!=( const address& a, const address& b );

        /**
         *  @return true if the ip is in the following ranges:
         *
         *  * 10.0.0.0    to 10.255.255.255
         *  * 172.16.0.0  to 172.31.255.255
         *  * 192.168.0.0 to 192.168.255.255
         *  * 169.254.0.0 to 169.254.255.255
         *
         */
        bool is_private_address()const;
        /**
         *  224.0.0.0 to 239.255.255.255
         */
        bool is_multicast_address()const;
        /**
         *  127.0.0.0 to 127.255.255.255
         */
        bool is_loopback_address()const;

        /**
         * not private, not loopback, and not multicast
         *
         * @note technically there are some addresses that are reserved but not public
         *   (see https://en.wikipedia.org/wiki/Reserved_IP_addresses), but so far we haven't distinguished them.
         * * 0.0.0.0–0.255.255.255
         * * 100.64.0.0–100.127.255.255
         * * 192.0.0.0–192.0.0.255
         * * 192.0.2.0–192.0.2.255
         * * 192.88.99.0–192.88.99.255
         * * 198.18.0.0–198.19.255.255
         * * 198.51.100.0–198.51.100.255
         * * 203.0.113.0–203.0.113.255
         * * 233.252.0.0-233.252.0.255 (within the range 224.0.0.0–239.255.255.255)
         * * 240.0.0.0–255.255.255.254
         * * 255.255.255.255
         */
        bool is_public_address()const;
      private:
        uint32_t _ip;
    };

    class endpoint {
      public:
        endpoint() = default;
        explicit endpoint( const address& i, uint16_t p = 0);
        explicit endpoint( uint32_t i, uint16_t p = 0);

        /** Converts "IP:PORT" to an endpoint */
        static endpoint from_string( const string& s );
        /** returns "IP:PORT" */
        explicit operator string()const;

        void           set_port(uint16_t p ) { _port = p; }
        uint16_t       port()const;
        const address& get_address()const;

        friend bool operator==( const endpoint& a, const endpoint& b );
        friend bool operator!=( const endpoint& a, const endpoint& b );
        friend bool operator< ( const endpoint& a, const endpoint& b );

      private:
        /**
         *  The compiler pads endpoint to a full 8 bytes, so while
         *  a port number is limited in range to 16 bits, we specify
         *  a full 32 bits so that memcmp can be used with sizeof(),
         *  otherwise 2 bytes will be 'random' and you do not know
         *  where they are stored.
         */
        uint32_t _port = 0;
        address  _ip;
    };

  }
  class variant;
  void to_variant( const ip::endpoint& var,  variant& vo, uint32_t _max_depth = 2 );
  void from_variant( const variant& var,  ip::endpoint& vo, uint32_t _max_depth = 2 );

  void to_variant( const ip::address& var,  variant& vo, uint32_t _max_depth = 1 );
  void from_variant( const variant& var,  ip::address& vo, uint32_t _max_depth = 1 );


  namespace raw
  {
    template<typename Stream>
    void pack( Stream& s, const ip::address& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH )
    {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( s, uint32_t(v), _max_depth - 1 );
    }
    template<typename Stream>
    void unpack( Stream& s, ip::address& v, uint32_t _max_depth=FC_PACK_MAX_DEPTH )
    {
       FC_ASSERT( _max_depth > 0 );
       uint32_t _ip;
       fc::raw::unpack( s, _ip, _max_depth - 1 );
       v = ip::address(_ip);
    }

    template<typename Stream>
    inline void pack( Stream& s, const ip::endpoint& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, v.get_address(), _max_depth );
       fc::raw::pack( s, v.port(), _max_depth );
    }
    template<typename Stream>
    inline void unpack( Stream& s, ip::endpoint& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       ip::address a;
       uint16_t p;
       fc::raw::unpack( s, a, _max_depth );
       fc::raw::unpack( s, p, _max_depth );
       v = ip::endpoint(a,p);
    }

  }
} // namespace fc
FC_REFLECT_TYPENAME( fc::ip::address )
FC_REFLECT_TYPENAME( fc::ip::endpoint )
namespace std
{
    template<>
    struct hash<fc::ip::endpoint>
    {
       size_t operator()( const fc::ip::endpoint& e )const
       {
           return fc::city_hash_size_t( (char*)&e, sizeof(e) );
       }
    };
}
