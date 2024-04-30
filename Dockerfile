# c++ runtime
FROM gcc:latest

# copy the current directory contents into the container at /app
ADD . /app

# Install libxlsxwriter
RUN mkdir /app/libs
RUN git clone https://github.com/jmcnamara/libxlsxwriter.git /app/libs/libxlsxwriter
RUN (cd /app/libs/libxlsxwriter && make)
RUN (cd /app/libs/libxlsxwriter && make install)
ENV LD_LIBRARY_PATH=/app/libs/libxlsxwriter/lib:${LD_LIBRARY_PATH}
RUN echo '/usr/local/lib' | tee -a /etc/ld.so.conf.d/x86_64-linux-gnu.conf && ldconfig

# curl and python3 for testing requests (see README)
# vim because reasons
# libcurl library for c++ code that sends http requests
# rapidjson library for interpreting json response
RUN apt-get update && apt-get install -y curl vim python3-pip rapidjson-dev libcurl4-openssl-dev sqlite3 libsqlite3-dev build-essential