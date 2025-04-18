ip=$(resolvectl status | grep "DNS Server" | head -n 1 | awk '{print $4}')
echo $ip
sudo tcpdump -i any port 53 and host $ip
