#### Author : Sagnik Bhattacharya 
#### Subject : Cloud comp bonus point 
#### Question : Create the entire infra using any IaC software which will automatically scale based on the load



# Initialize the provider and authenticate with Azure
provider "azurerm" {
  features = {}
}

# Define the resource group
resource "azurerm_resource_group" "rg" {
  name     = "myResourceGroup"
  location = "East US"
}

# Create a virtual network
resource "azurerm_virtual_network" "vnet" {
  name                = "myVNet"
  address_space       = ["10.0.0.0/16"]
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
}

# Create a public subnet
resource "azurerm_subnet" "public_subnet" {
  name                 = "myPublicSubnet"
  resource_group_name  = azurerm_resource_group.rg.name
  virtual_network_name = azurerm_virtual_network.vnet.name
  address_prefixes     = ["10.0.1.0/24"]
}

# Create a private subnet
resource "azurerm_subnet" "private_subnet" {
  name                 = "myPrivateSubnet"
  resource_group_name  = azurerm_resource_group.rg.name
  virtual_network_name = azurerm_virtual_network.vnet.name
  address_prefixes     = ["10.0.2.0/24"]
}

# Network Interface for the VM in the private subnet
resource "azurerm_network_interface" "private_nic" {
  name                = "myPrivateNic"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  ip_configuration {
    name                          = "internal"
    subnet_id                     = azurerm_subnet.private_subnet.id
    private_ip_address_allocation = "Dynamic"
  }
}

# Create a Virtual Machine in the private subnet
resource "azurerm_linux_virtual_machine" "vm_private" {
  name                = "myPrivateVM"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  network_interface_ids = [
    azurerm_network_interface.private_nic.id,
  ]
  size               = "Standard_DS1_v2"
  admin_username     = "azureuser"
  admin_password     = "Password1234!"
  disable_password_authentication = false

  os_disk {
    caching              = "ReadWrite"
    storage_account_type = "Standard_LRS"
  }

  source_image_reference {
    publisher = "Canonical"
    offer     = "UbuntuServer"
    sku       = "18.04-LTS"
    version   = "latest"
  }
}

# Create MySQL Server in the private subnet
resource "azurerm_mysql_server" "mysql" {
  name                = "mysqlserver"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  administrator_login = "mysqladmin"
  administrator_login_password = "Password1234!"
  sku_name            = "B_Gen5_1"
  version             = "5.7"
  storage_mb          = 5120
  backup_retention_days = 7
  geo_redundant_backup = "Disabled"
  auto_grow           = "Enabled"
  infrastructure_encryption_enabled = false
  public_network_access_enabled = false

  ssl_enforcement_enabled = false
  ssl_minimal_tls_version_enforced = "TLS1_2"
  virtual_network_subnet_id        = azurerm_subnet.private_subnet.id
}

# Public Network Interface for the VM in the public subnet
resource "azurerm_network_interface" "public_nic" {
  name                = "myPublicNic"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  ip_configuration {
    name                          = "internal"
    subnet_id                     = azurerm_subnet.public_subnet.id
    private_ip_address_allocation = "Dynamic"
    public_ip_address_id          = azurerm_public_ip.public_ip.id
  }
}

# Create a Public IP for the VM
resource "azurerm_public_ip" "public_ip" {
  name                = "myPublicIP"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  allocation_method   = "Dynamic"
}

# Create a Virtual Machine in the public subnet
resource "azurerm_linux_virtual_machine" "vm_public" {
  name                = "myPublicVM"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  network_interface_ids = [
    azurerm_network_interface.public_nic.id,
  ]
  size               = "Standard_DS1_v2"
  admin_username     = "azureuser"
  admin_password     = "Password1234!"
  disable_password_authentication = false

  os_disk {
    caching              = "ReadWrite"
    storage_account_type = "Standard_LRS"
  }

  source_image_reference {
    publisher = "Canonical"
    offer     = "UbuntuServer"
    sku       = "18.04-LTS"
    version   = "latest"
  }
}

# Create Load Balancer
resource "azurerm_lb" "lb" {
  name                = "myLoadBalancer"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  frontend_ip_configuration {
    name                 = "PublicIPAddress"
    public_ip_address_id = azurerm_public_ip.public_ip.id
  }
}

# Create Load Balancer Probe for health monitoring
resource "azurerm_lb_probe" "probe" {
  resource_group_name = azurerm_resource_group.rg.name
  loadbalancer_id     = azurerm_lb.lb.id
  name                = "httpProbe"
  protocol            = "Http"
  port                = 80
  request_path        = "/"
  interval_in_seconds = 5
  number_of_probes    = 2
}

# Add VM to Load Balancer backend pool
resource "azurerm_lb_backend_address_pool" "backend_pool" {
  resource_group_name = azurerm_resource_group.rg.name
  loadbalancer_id     = azurerm_lb.lb.id
  name                = "backendPool"
}

# Create Load Balancer rule
resource "azurerm_lb_rule" "lb_rule" {
  resource_group_name            = azurerm_resource_group.rg.name
  loadbalancer_id                = azurerm_lb.lb.id
  name                           = "HTTP"
  protocol                       = "Tcp"
  frontend_port                  = 80
  backend_port                   = 80
  frontend_ip_configuration_name = azurerm_lb.lb.frontend_ip_configuration[0].name
  backend_address_pool_id        = azurerm_lb_backend_address_pool.backend_pool.id
  probe_id                       = azurerm_lb_probe.probe.id
}

# Associate the VM with the Load Balancer backend pool
resource "azurerm_network_interface_backend_address_pool_association" "vm_public_lb" {
  network_interface_id            = azurerm_network_interface.public_nic.id
  backend_address_pool_id         = azurerm_lb_backend_address_pool.backend_pool.id
}

# Auto-scaling based on CPU or RAM utilization
resource "azurerm_monitor_autoscale_setting" "autoscale" {
  name                = "autoscale-setting"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  target_resource_id  = azurerm_linux_virtual_machine.vm_private.id

  profile {
    name = "defaultProfile"
    capacity {
      minimum = 1
      maximum = 5
      default = 1
    }

    rule {
      metric_trigger {
        metric_name        = "Percentage CPU"
        metric_resource_id = azurerm_linux_virtual_machine.vm_private.id
        time_grain         = "PT1M"
        statistic          = "Average"
        time_window        = "PT10M"
        time_aggregation   = "Average"
        operator           = "GreaterThan"