#######################################################
#### Incomplete
####
#### By Sagnik Bhattacharya, 2024
####
#######################################################

terraform {
  required_providers {
    openstack = {
      source  = "terraform-provider-openstack/openstack"
      version = "~> 1.0"
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
}

provider "kubernetes" {
  host                   = var.kubernetes_host
  client_certificate     = file(var.client_certificate)
  client_key             = file(var.client_key)
  cluster_ca_certificate = file(var.cluster_ca_certificate)
}

# Define variables without default values
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

variable "ssh_public_key" {
  description = "Path to the SSH public key"
  type        = string
}

variable "kubernetes_host" {
  description = "Kubernetes API server URL"
  type        = string
}

variable "client_certificate" {
  description = "Path to the client certificate for Kubernetes"
  type        = string
}

variable "client_key" {
  description = "Path to the client key for Kubernetes"
  type        = string
}

variable "cluster_ca_certificate" {
  description = "Path to the cluster CA certificate for Kubernetes"
  type        = string
}

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
