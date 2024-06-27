## These are for connecting with Openstack and sharing the Keypair

auth_url        = "https://10.32.4.182:5000/v3"
region          = "RegionOne"
tenant_name     = "CloudComp10" # Also known as project 
user_name       = "CloudComp10"
password        = "demo"
domain_name     = "default"
# ssh_public_key  = "~/.ssh/id_ecdsa.pub"

# These are needed for the internal SSL certificate
# Must use for Pord env but for simplicity removed from here 

# client_certificate = "~/.ssh/client.crt"
# client_key = "~/.ssh/client.key"
# cluster_ca_certificate = "~/.ssh/ca.crt"

# Instance Configuration
# num_worker_nodes is < 0 then master will be the worker otherwise master
# is only for control

num_worker_nodes = 3
master_flavor    = "m1.small"
worker_flavor    = "m1.medium"
os_image         = "ubuntu-22.04-jammy-x86_64"
volume_size      = 15
dns_servers     = ["10.33.16.100"]
floating_ip_pool = "ext_net"
delay_seconds = 120