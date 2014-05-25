alter table tblloginserveraccounts add column `client_unlock` tinyint(3) NOT NULL DEFAULT '0';
alter table tblloginserveraccounts add column `created_by` tinyint(3) NOT NULL DEFAULT '0';