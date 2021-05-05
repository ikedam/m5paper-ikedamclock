FROM debian:buster-slim

WORKDIR /workspace

RUN apt-get update \
  && apt-get install -y --no-install-recommends fontforge \
  && apt-get -y clean \
  && rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["fontforge"]
