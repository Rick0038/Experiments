#######################################################
#### THIS IS NOT HOW YOU DEPLOY K3S IN PROD
#### THIS DOES NOT USE CERTS FOR INTERNAL COMMUNICATION
#### USE THE SECURE SCRIPT FOR ACTUAL DEPLOYMENT
####
#### By Sagnik Bhattacharya, 2024
####
#######################################################

# instaling dependency
terraform {
  required_version = ">= 0.14.0"
  required_providers {
    openstack = {
      source  = "terraform-provider-openstack/openstack"
      version = ">= 2.0.0"
    }
    tls = {
      source  = "hashicorp/tls"
      version = ">= 3.1.0"
    }
    kubernetes = {
      source  = "hashicorp/kubernetes"
      version = "~> 2.0"
    }
  }
}

provider "openstack" {
  auth_url    = var.auth_url
  region      = var.region
  tenant_name = var.tenant_name
  user_name   = var.user_name
  password    = var.password
  domain_name = var.domain_name
  insecure    = true # DANGER
}

variable "auth_url" {
  description = "OpenStack authentication URL"
  type        = string
}

variable "region" {
  description = "OpenStack region"
  type        = string
}

variable "tenant_name" {
  description = "OpenStack tenant name"
  type        = string
}

variable "user_name" {
  description = "OpenStack username"
  type        = string
}

variable "password" {
  description = "OpenStack password"
  type        = string
  sensitive   = true
}

variable "domain_name" {
  description = "OpenStack domain name"
  type        = string
}

# Broken for some reason dont know why 
# variable "ssh_public_key" {
#   description = "Path to the SSH public key"
#   type        = string
# }

variable "num_worker_nodes" {
  description = "Number of worker nodes to create"
  type        = number
}

variable "master_flavor" {
  description = "Flavor for the master node"
  type        = string
}

variable "worker_flavor" {
  description = "Flavor for the worker nodes"
  type        = string
}

variable "os_image" {
  description = "OS image to use for instances"
  type        = string
}

variable "volume_size" {
  description = "Size of the volumes to create for nodes"
  type        = number
}

variable "dns_servers" {
  description = "List of DNS servers for the instances"
  type        = list(string)
}

variable "floating_ip_pool" {
  description = "Name of the floating IP pool for the instances"
  type        = string
}

variable "delay_seconds" {
  description = "The delay in seconds before creating the worker nodes"
  default     = 120
}

# Delay resource for master
resource "null_resource" "delay_master" {
  provisioner "local-exec" {
    command = "sleep ${var.delay_seconds}"
  }
  triggers = {
    instance_id_master = openstack_compute_instance_v2.k3s_master.id
  }
}

# Delay resource for workers
resource "null_resource" "delay_workers" {
  provisioner "local-exec" {
    command = "sleep ${var.delay_seconds}"
  }
  triggers = {
    instance_id_workers = join(",", openstack_compute_instance_v2.k3s_workers.*.id)
  }
}


# Define the network
resource "openstack_networking_network_v2" "network" {
  name           = "k3s-network"
  admin_state_up = "true"
}

# Define the subnet
resource "openstack_networking_subnet_v2" "subnet" {
  name       = "k3s-subnet"
  network_id = openstack_networking_network_v2.network.id
  cidr       = "192.168.1.0/24"
  ip_version = 4
  dns_nameservers = var.dns_servers
}

# Define the router

data "openstack_networking_network_v2" "floating_ip" {
  name = var.floating_ip_pool
}

resource "openstack_networking_router_v2" "router" {
  name             = "k3s-router"
  admin_state_up   = "true"
  external_network_id  = data.openstack_networking_network_v2.floating_ip.id
}

# Connect the router to the subnet
resource "openstack_networking_router_interface_v2" "router_interface" {
  router_id = openstack_networking_router_v2.router.id
  subnet_id = openstack_networking_subnet_v2.subnet.id
}

# Adding FIP to master ## DEPRICATED 
resource "openstack_networking_floatingip_v2" "fip" {
  pool = var.floating_ip_pool
}

resource "openstack_compute_floatingip_associate_v2" "fip_assoc" {
  floating_ip = openstack_networking_floatingip_v2.fip.address
  instance_id = openstack_compute_instance_v2.k3s_master.id
}


# Creating SSH keys
resource "tls_private_key" "ssh" {
  algorithm = "ECDSA"
  ecdsa_curve = "P256"
}

# Saving key in local
resource "local_file" "private_key" {
  content  = tls_private_key.ssh.private_key_pem
  filename = "${path.module}/id_rsa"
}

# Define the keypair for SSH
resource "openstack_compute_keypair_v2" "default" {
  name       = "k3s-key"
  # public_key = file(var.ssh_public_key)
  public_key = tls_private_key.ssh.public_key_openssh
}

# Create a new security group
resource "openstack_networking_secgroup_v2" "secgroup" {
  name        = "k3s-secgroup"
  description = "Security group for k3s"
}

# # Allow SSH traffic
# resource "openstack_networking_secgroup_rule_v2" "secgroup_rule_ssh" {
#   direction         = "ingress"
#   ethertype         = "IPv4"
#   protocol          = "tcp"
#   port_range_min    = 22
#   port_range_max    = 22
#   remote_ip_prefix  = "0.0.0.0/0"
#   security_group_id = openstack_networking_secgroup_v2.secgroup.id
# }

########### DONT DO THIS ITS VERY BAD ########################
# Allow all inbound traffic

resource "openstack_networking_secgroup_rule_v2" "secgroup_rule_all_inbound" {
  direction         = "ingress"
  ethertype         = "IPv4"
  remote_ip_prefix  = "0.0.0.0/0"
  security_group_id = openstack_networking_secgroup_v2.secgroup.id
}
#############################################################


# Allow all outbound traffic
resource "openstack_networking_secgroup_rule_v2" "secgroup_rule_all_outbound" {
  direction         = "egress"
  ethertype         = "IPv4"
  remote_ip_prefix  = "0.0.0.0/0"
  security_group_id = openstack_networking_secgroup_v2.secgroup.id
}

# Define the master node
resource "openstack_compute_instance_v2" "k3s_master" {
  name          = "kube-master"
  image_name    = var.os_image
  flavor_name   = var.master_flavor
  key_pair      = openstack_compute_keypair_v2.default.name
  security_groups = ["default",openstack_networking_secgroup_v2.secgroup.name]
  network {
    uuid = openstack_networking_network_v2.network.id
  }

  # This thing does all the magic, a glorified bash script XD
  user_data = <<-EOT
              #!/bin/bash
              apt-get update
              apt-get install -y curl
              echo "Before snap"
              snap install helm --classic 
              
              # Install KubeCTL
              curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl"
              install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl
              kubectl version --client
              echo "before K3S"
              
              # Install K3s with taint if there are worker nodes
              if [ ${var.num_worker_nodes} -gt 0 ]; then
                curl -sfL https://get.k3s.io | sh -s - --node-taint key=value:NoExecute --disable traefik --disable-agent --tls-san 127.0.0.1
              else
                # Install K3s without taint, allowing the master to schedule pods
                curl -sfL https://get.k3s.io | sh -s - --disable traefik --disable-agent --tls-san 127.0.0.1
              fi

              # Wait and save the token into a file
              while [ ! -f /var/lib/rancher/k3s/server/node-token ]; do
                sleep 5
              done
              mkdir -p /var/lib/rancher/k3s/server/
              echo $(cat /var/lib/rancher/k3s/server/node-token) > /var/lib/rancher/k3s/server/token
              chmod 777 /var/lib/rancher/k3s/server/token
              ls -ltr /var/lib/rancher/k3s/server/token

              # Mount the volume at /mnt
              mkdir /mnt/data
              mkfs.ext4 /dev/vdb
              echo '/dev/vdb /mnt/data ext4 defaults 0 0' >> /etc/fstab
              mount -a

              # Adding kubeconfig
              chmod 644 /etc/rancher/k3s/k3s.yaml
              echo "export KUBECONFIG=/etc/rancher/k3s/k3s.yaml" >> /etc/profile

              EOT

  metadata = {
    instance_role = "master"
  }
}

# Define the volume for the master node
resource "openstack_blockstorage_volume_v3" "k3s_master_volume" {
  name = "k3s-master-volume"
  size = var.volume_size
}

# Attach the volume to the master node
resource "openstack_compute_volume_attach_v2" "k3s_master_volume_attach" {
  instance_id = openstack_compute_instance_v2.k3s_master.id
  volume_id   = openstack_blockstorage_volume_v3.k3s_master_volume.id
}

resource "openstack_compute_instance_v2" "k3s_workers" {
  count         = var.num_worker_nodes
  name          = "kubeworker-${count.index}"
  image_name    = var.os_image
  flavor_name   = var.worker_flavor
  key_pair      = openstack_compute_keypair_v2.default.name
  security_groups = ["default", openstack_networking_secgroup_v2.secgroup.name]
  depends_on    = [
                    openstack_compute_instance_v2.k3s_master,
                    null_resource.delay_master
                  ]

  network {
    uuid = openstack_networking_network_v2.network.id
  }

  # This script installs necessary software and prepares the mount point
  user_data = <<-EOT
              #!/bin/bash
              echo "hello"
              apt-get update
              apt-get install -y curl
              
              # Create a mount point for the attached volume
              mkdir /mnt/data
              mkfs.ext4 /dev/vdb
              echo '/dev/vdb /mnt/data ext4 defaults 0 0' >> /etc/fstab
              mount -a

              # Save the private key
              echo '${tls_private_key.ssh.private_key_pem}' > /home/ubuntu/.ssh/id_rsa
              chmod 600 /home/ubuntu/.ssh/id_rsa
              while [ -z "$TOKEN" ]; do
                TOKEN=$(ssh -o StrictHostKeyChecking=no -i /home/ubuntu/.ssh/id_rsa ubuntu@${openstack_compute_instance_v2.k3s_master.network.0.fixed_ip_v4} 'sudo cat /var/lib/rancher/k3s/server/token')
                sleep 5
              done
              curl -sfL https://get.k3s.io | K3S_URL=https://${openstack_compute_instance_v2.k3s_master.network.0.fixed_ip_v4}:6443 K3S_TOKEN=$TOKEN sh -
              EOT

  # provisioner "remote-exec" {
  #   inline = [
  #     "TOKEN=$(ssh -o StrictHostKeyChecking=no -l ubuntu ${openstack_compute_instance_v2.k3s_master.network.0.fixed_ip_v4} 'cat /var/lib/rancher/k3s/server/token')",
  #     "curl -sfL https://get.k3s.io | K3S_URL=http://${openstack_compute_instance_v2.k3s_master.network.0.fixed_ip_v4}:6443 K3S_TOKEN=$TOKEN sh -"
  #   ]

    connection {
      type        = "ssh"
      user        = "ubuntu"
      private_key = tls_private_key.ssh.private_key_pem
      host        = self.access_ip_v4
    }

  metadata = {
    instance_role = "worker"
  }
}

# Define the volumes for the worker nodes
resource "openstack_blockstorage_volume_v3" "k3s_worker_volumes" {
  count = var.num_worker_nodes
  name  = "k3s-worker-volume-${count.index}"
  size  = var.volume_size
}

# Attach the volumes to the worker nodes
resource "openstack_compute_volume_attach_v2" "k3s_worker_volume_attach" {
  count       = var.num_worker_nodes
  instance_id = element(openstack_compute_instance_v2.k3s_workers.*.id, count.index)
  volume_id   = element(openstack_blockstorage_volume_v3.k3s_worker_volumes.*.id, count.index)
  
  # Ensure attachment only happens after instance and volume creation
  depends_on = [
    openstack_compute_instance_v2.k3s_workers,
    openstack_blockstorage_volume_v3.k3s_worker_volumes
  ]
}


data "kubernetes_namespace" "existing" {
  metadata {
    name = "kube-system"
  }
}

resource "kubernetes_namespace" "default" {
  count = data.kubernetes_namespace.existing.id != null ? 0 : 1
  depends_on = [null_resource.delay_workers]
  metadata {
    name = "kube-system"
  }
}


resource "kubernetes_deployment" "traefik" {
  metadata {
    name      = "traefik"
    namespace = "kube-system"
    labels = {
      app = "traefik"
    }
  }

  spec {
    replicas = 1
    selector {
      match_labels = {
        app = "traefik"
      }
    }

    template {
      metadata {
        labels = {
          app = "traefik"
        }
      }

      spec {
        container {
          name  = "traefik"
          image = "traefik:v2.4"
          args  = ["--providers.kubernetescrd", "--entrypoints.web.Address=:80", "--entrypoints.websecure.Address=:443"]

          port {
            name = "web"
            container_port = 80
          }

          port {
            name = "websecure"
            container_port = 443
          }
        }
      }
    }
  }
}

resource "kubernetes_service" "traefik" {
  metadata {
    name      = "traefik"
    namespace = "kube-system"
    labels = {
      app = "traefik"
    }
  }

  spec {
    selector = {
      app = "traefik"
    }

    type = "LoadBalancer"

    port {
      name = "web"
      port        = 80
      target_port = 80
    }

    port {
      name = "websecure"
      port        = 443
      target_port = 443
    }
  }
}

output "traefik_lb_ip" {
  value = flatten([for s in kubernetes_service.traefik.status : [for i in s.load_balancer.ingress : i.ip]])
}
