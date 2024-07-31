FROM nixos/nix:2.21.4

COPY . /tmp/build
WORKDIR /tmp/build

RUN echo "experimental-features = nix-command flakes" >> /etc/nix/nix.conf && nix develop

ENTRYPOINT ["nix", "develop"]
