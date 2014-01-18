/*
Navicat MySQL Data Transfer

Source Server         : localhost_3308
Source Server Version : 50609
Source Host           : localhost:3306
Source Database       : eqlogin

Target Server Type    : MYSQL
Target Server Version : 50609
File Encoding         : 65001

Date: 2013-08-08 14:56:39
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `variables`
-- ----------------------------
DROP TABLE IF EXISTS `variables`;
CREATE TABLE `variables` (
  `varname` varchar(25) NOT NULL DEFAULT '',
  `value` text NOT NULL,
  `information` text NOT NULL,
  `ts` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`varname`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of variables
-- ----------------------------
INSERT INTO `variables` VALUES ('AAXPMod', '0.75', 'AA Experience multipler. Increase to increase exp rate', '2007-10-12 19:14:12');
INSERT INTO `variables` VALUES ('ACfail', '15', 'The percentage of time AC fails to protect. 0 would mean there was always some level of protection, 100 would mean AC has no affect. When AC fails, it will be possible to get a max dmg hit.', '2008-01-31 22:21:54');
INSERT INTO `variables` VALUES ('ACrandom', '20', 'These 3 AC rules DO NOT apply if you have the newer AC rule enabled in rule_values! - the maximum amount of additional protection AC provides. 0 would mean no additional protection is provided, otherwise an additional amount of reduction is calculated using a random percentage of 1 to this value (except when AC fails with ACfail)', '2008-01-31 22:19:31');
INSERT INTO `variables` VALUES ('ACreduction', '3', 'The percentage of AC that is ALWAYS reduced from a hit (except when AC fails with ACfail) ', '2008-01-31 22:19:23');
INSERT INTO `variables` VALUES ('ailevel', '6', 'Had something to do with NPC casting. Leave at 6.', '2008-01-31 22:21:34');
INSERT INTO `variables` VALUES ('DBVersion', '070_pop_cvs', 'DB version info', '2008-01-31 22:23:03');
INSERT INTO `variables` VALUES ('DisableNoDrop', '0', 'Takes No-Drop off of items', '2004-10-28 15:08:04');
INSERT INTO `variables` VALUES ('LootCoin', '1', 'Allows players to loot coin off a player corpse in pvp | 0 - off | 1 - on |', '2006-03-27 22:42:49');
INSERT INTO `variables` VALUES ('disablecommandline', '0', 'Allow command lines to be run from world.exe | 0 - off | 1 - on |', '2007-02-13 18:59:01');
INSERT INTO `variables` VALUES ('Expansions', '16383', 'Accessible expansions for each player', '2007-02-13 18:59:01');
INSERT INTO `variables` VALUES ('ServerType', '0', 'Sets server type | 0 - normal | 1 - pvp |', '2004-10-28 15:09:41');
INSERT INTO `variables` VALUES ('holdzones', '1', 'Restart Crashed Zone Servers | 0 - off | 1 - on |', '2008-01-31 22:20:29');
INSERT INTO `variables` VALUES ('Max_AAXP', '21626880', 'Max AA Experience', '2007-02-13 18:59:01');
INSERT INTO `variables` VALUES ('MerchantsKeepItems', '0', 'Merchants keep items sold to them | 0 - off | 1 - on |', '2007-02-13 18:59:01');
INSERT INTO `variables` VALUES ('MOTD', 'CAT PLANET!!!!!!!!!!!', 'Server Message of the Day', '2013-01-27 19:51:40');
INSERT INTO `variables` VALUES ('GroupEXPBonus', '0.60', 'Experience multipler. Increase to increase group exp rate', '2007-11-17 21:20:43');
INSERT INTO `variables` VALUES ('PvPreward', '0', 'Allows players to loot items off a player corpse in pvp | 0 - no items | 1 - a single item | 2 - all items | 3 - item specified in PvPreward |', '2004-11-08 12:28:35');
INSERT INTO `variables` VALUES ('PvPitem', '0', 'Specific item that can be looted off a player in pvp', '2004-11-08 12:28:08');
INSERT INTO `variables` VALUES ('LoginVersion', '12-4-2002 1800', 'Client Version for LoginServer', '2013-08-08 14:55:51');
