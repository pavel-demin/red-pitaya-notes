apt-get install libpcre3-dev libluajit-5.1-dev libcurl4-openssl-dev libssl-dev libboost-regex1.55-dev libboost-system1.55-dev libboost-thread1.55-dev libcrypto++-dev zlib1g-dev

curl -L https://github.com/RedPitaya/RedPitaya/archive/v0.95-1.tar.gz -o RedPitaya-0.95-1.tar.gz
tar -zxf RedPitaya-0.95-1.tar.gz
cd RedPitaya-0.95-1

cat <<- EOF_CAT > patches/lua-nginx-module.patch
diff -rupN lua-nginx-module.old/config lua-nginx-module.new/config
--- lua-nginx-module.old/config
+++ lua-nginx-module.new/config
@@ -295,8 +311,8 @@ fi
 NGX_DTRACE_PROVIDERS="$NGX_DTRACE_PROVIDERS $ngx_addon_dir/dtrace/ngx_lua_provider.d"
 NGX_TAPSET_SRCS="$NGX_TAPSET_SRCS $ngx_addon_dir/tapset/ngx_lua.stp"

-USE_MD5=YES
-USE_SHA1=YES
+USE_MD5=NO
+USE_SHA1=NO

 CORE_INCS="$CORE_INCS $ngx_addon_dir/src/api"

diff -rupN lua-nginx-module.old/src/ngx_http_lua_socket_tcp.c lua-nginx-module.new/src/ngx_http_lua_socket_tcp.c
--- lua-nginx-module.old/src/ngx_http_lua_socket_tcp.c
+++ lua-nginx-module.new/src/ngx_http_lua_socket_tcp.c
@@ -3146,8 +3146,7 @@ ngx_http_lua_req_socket(lua_State *L)
     }

     lua_settop(L, 1);
-    lua_pushnil(L);
-    return 2;
+    return 1;
 }


EOF_CAT

make Bazaar/tools/libjson
make Bazaar/nginx/ngx_ext_modules/ws_server/websocketpp
make Bazaar/nginx/ngx_ext_modules/lua-nginx-module
make Bazaar/nginx/nginx-1.5.3

make -C shared

patch -d Bazaar/nginx/ngx_ext_modules/ws_server/rp_sdk -p1 <<- EOF_PATCH
diff -rupN rp_sdk.old/Makefile rp_sdk.new/Makefile
--- rp_sdk.old/Makefile
+++ rp_sdk.new/Makefile
@@ -22,7 +22,7 @@ CRYPTO_DIR=../../../../tools/cryptopp
 CRYPTO_INSTALL_DIR=../../../../tools/build

 CXX=\$(CROSS_COMPILE)g++
-CXXFLAGS=-c -Wall -O0 -static -std=c++11 -fPIC -I\$(LIBJSON_DIR) -I\$(CRYPTO_INSTALL_DIR)/include/cryptopp -DNDEBUG
+CXXFLAGS=-c -Wall -O0 -static -std=c++11 -fPIC -I\$(LIBJSON_DIR) -I/usr/include/crypto++ -DNDEBUG
 ifeq (\$(ALWAYS_PURCHASED),true)
 CXXFLAGS+=-DALWAYS_PURCHASED
 endif
@@ -51,9 +51,7 @@ \$(CRYPTO_LIB):
 	make -C \$(CRYPTO_DIR) CXX=\${CROSS_COMPILE}g++ PREFIX=../build install

 \$(LIB): \$(OBJECTS)
-	mkdir -p \$(OBJDIR)/cryptopp
-	cd \$(OBJDIR)/cryptopp; ar -x \$(CURRENT_DIR)/\$(CRYPTO_LIB)
-	ar rc \$(LIB) objs/cryptopp/*.o \$(OBJECTS)
+	ar rc \$(LIB) \$(OBJECTS)

 \$(SDKOBJDIR)/%.o: %.cpp
 	mkdir -p \$(dir \$@)
EOF_PATCH

make -C Bazaar/nginx/ngx_ext_modules/ws_server/rp_sdk librp_sdk.a

patch -d Bazaar/nginx/ngx_ext_modules/ws_server/websocketpp/websocketpp -p1 <<- EOF_PATCH
diff -rupN websocketpp.old/transport/asio/endpoint.hpp websocketpp.new/transport/asio/endpoint.hpp
--- websocketpp.old/transport/asio/endpoint.hpp
+++ websocketpp.new/transport/asio/endpoint.hpp
@@ -95,7 +95,7 @@ public:
     explicit endpoint()
       : m_io_service(NULL)
       , m_external_io_service(false)
-      , m_listen_backlog(0)
+      , m_listen_backlog(boost::asio::socket_base::max_connections)
       , m_reuse_addr(false)
       , m_state(UNINITIALIZED)
     {
EOF_PATCH

make -C Bazaar/nginx/ngx_ext_modules/ws_server

cd Bazaar/nginx/nginx-1.5.3
./configure `cat ../configure_withouts.txt` --add-module=../ngx_ext_modules/lua-nginx-module --add-module=../ngx_ext_modules/ngx_http_rp_module
make
cp objs/nginx /opt/redpitaya/sbin/nginx
strip /opt/redpitaya/sbin/nginx
cd -

patch -d Applications/scopegenpro/src -p1 <<- EOF_PATCH
diff -rupN src.old/Makefile src.new/Makefile
--- src.old/Makefile
+++ src.new/Makefile
@@ -3,7 +3,7 @@ RM=rm

 CXXSOURCES=main.cpp

-RP_API=../../../api/lib
+RP_API=/opt/redpitaya/lib
 RP_SDK=../../../Bazaar/nginx/ngx_ext_modules/ws_server/rp_sdk

 INCLUDE = -I\$(RP_SDK)
@@ -14,7 +14,7 @@ COMMON_FLAGS+=-Wall -fPIC -lstdc++ -Os -s
 CXXFLAGS+=\$(COMMON_FLAGS) -std=c++11 \$(INCLUDE)
 LDFLAGS =-shared \$(COMMON_FLAGS) -L\$(RP_SDK)/lib
 LDFLAGS+= -Wl,--whole-archive
-LDFLAGS+=-L\$(RP_SDK) -lrp_sdk
+LDFLAGS+=-L\$(RP_SDK) -lrp_sdk -lcryptopp
 LDFLAGS+=-L\$(RP_API) -lrpapp -lrp
 LDFLAGS+= -Wl,--no-whole-archive

EOF_PATCH

make -C Applications/scopegenpro/src
cp Applications/scopegenpro/controllerhf.so /opt/redpitaya/www/apps/scopegenpro/controllerhf.so
