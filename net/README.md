# Networking layer

The networking layer implements the following protocols:

- Ethernet: the basic protocol to be able to communicate over the local network
   - ARP (Address Resolution Protocol): converts an IP address into a MAC address. Type `arp` to see the content of the ARP table
   - IPv4 (Internet Protocol Version 4)
      - ICMP (Internet Control Message Protocol): used for the `ping` command
      - UDP (User Datagram Protocol)
         - DHCP (Dynamic Host Configuration Protocol): used at bootup time to get the network configuration from the local network router. Type `ifconfig` to see the network configuration
         - DNS (Domain Name System): used to convert a hostname into an IP address. Type `dns` to see the content of the DNS table
      - TCP (Transmission Control Protocol)
         - TLS 1.2 (for HTTPS connections), supporting the TLS_RSA_WITH_AES_CBC_128_SHA and TLS_DHE_RSA_WITH_AES_CBC_128_SHA cipher suites
           - HTTP 1.0 (HyperText Transfer Protocol): used by the `http` command

The networking stack relies on the PCI driver which scans the PCI bus (drivers/pci.c) as well as the Intel e1000 Ethernet adapter driver (drivers/e1000.c)
