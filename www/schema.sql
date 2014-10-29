
/* this script expects that a database has already been created */

USE `ensisoftcom`;

/*
DROP TABLE IF EXISTS `feedback`;
DROP TABLE IF EXISTS `import`; 
DROP TABLE IF EXISTS `newsflash`; 
*/

/* this table contains the feedback's received through the feedback.php web interface */
CREATE TABLE `feedback` (
       `id`          INT UNSIGNED NOT NULL AUTO_INCREMENT,
       `type`        TINYINT,         /* type of feedback, feedback/bugreport/feature request */
       `approved`    TINYINT,         /* approved comment has non-zero value */
       `name`        VARCHAR(255),    /* name of the person giving feedback */
       `email`       VARCHAR(255),    /* email of the person giving feedback */
       `host`        VARCHAR(255),    /* host is the host computer host-name or IP */
       `country`     VARCHAR(255),    /* country is the self reported country of the person giving feedback */
       `text`        VARCHAR(2048),   /* feedback text */
       `version`     VARCHAR(10),      /* application version, e.g. 3.2.0b1 */
       `platform`    VARCHAR(100),    /* platform, e.g. Windows XP, Win 7, Ubuntu 9.10 */
       `date`        TIMESTAMP DEFAULT NOW(),
       PRIMARY KEY (`id`)
       );

/* import counts for importing newznab settings */
CREATE TABLE `import` (
       `id`         INT UNSIGNED NOT NULL AUTO_INCREMENT,
       `platform`   VARCHAR(10),
       `host`       VARCHAR(255),
       `date`       TIMESTAMP DEFAULT NOW(),
       PRIMARY KEY(`id`)
       );

/* automatic update tracking */
CREATE TABLE `newsflash2` (
       `fingerprint` VARCHAR(64),
       `host`        VARCHAR(255),
       `platform`    VARCHAR(100),
       `version`     VARCHAR(10),
       `count`       INT UNSIGNED DEFAULT 0,
       `date`        TIMESTAMP DEFAULT NOW(),
       `license`     VARCHAR(64),
       `paypalref`   VARCHAR(20),
       `contributor` VARCHAR(255),
       PRIMARY KEY(`fingerprint`)
       );

