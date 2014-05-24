-- --------------------------------------------------------
-- Host:                         localhost
-- Server version:               5.6.16 - MySQL Community Server (GPL)
-- Server OS:                    Win32
-- HeidiSQL Version:             8.3.0.4694
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

-- Dumping structure for table macemu.tblloginserveraccounts
DROP TABLE IF EXISTS `tblloginserveraccounts`;
CREATE TABLE IF NOT EXISTS `tblloginserveraccounts` (
  `LoginServerID` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `AccountName` varchar(30) NOT NULL,
  `AccountPassword` varchar(50) NOT NULL,
  `AccountCreateDate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `AccountEmail` varchar(100) NOT NULL,
  `LastLoginDate` datetime NOT NULL,
  `LastIPAddress` varchar(15) NOT NULL,
  `client_unlock` varchar(5) NOT NULL DEFAULT 'false',
  `created_by` varchar(3) NOT NULL DEFAULT 'WEB',
  PRIMARY KEY (`LoginServerID`,`AccountName`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Data exporting was unselected.
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IF(@OLD_FOREIGN_KEY_CHECKS IS NULL, 1, @OLD_FOREIGN_KEY_CHECKS) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
