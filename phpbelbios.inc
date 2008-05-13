<?php

/***************************************************************************
 *   Copyright (C) 2008 by Bèr Kessels   *
 *   ber@webschuur.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

class Tixway {
  private $parameters;

  function __construct($user, $password, $parameters = array()) {
    $this->set_parameters($parameters);
    $this->set_parameters(array('user' => $user, 'password' => $password));
  }

  public function do_call() {
    $url = $this->build_url();
    dpr(TixwayUtil::urldecodeRFC3986($url));
    $data = $this->call_url($url);
    $xml = $this->parse_xml($data);
    return $xml;
  }

  public function build_url() {
    $this->add_default_parameters();//@todo make config with param.

    $out = $this->get_normalized_http_url() . "?";
    $out .= $this->to_postdata();
    return $out;
  }

  //@TODO: build in more and better fopen wrappers, such as curl support.
  public function call_url($url) {
    $xml = '';
    if ($handle = @fopen($url,'r')) {
      while (!feof($handle)) {
        $xml .= fgets($handle,256);
      }
      fclose($handle);
    }

    return $xml;
  }

  public function parse_xml($xml_string) {
    $xml_data = array();//Make sure we always return an array.
    if (!empty($xml_string) && (substr($xml_string, 0, 1) == '<')) { //RAS returns 0, in case of no results.
      if(!$simple = simplexml_load_string($xml_string)) {
        throw new Exception("XML is not valid: " . $xml_string);
      }

      foreach($simple->performance as $performance) {
        $xml_data[] = $performance;
      }
    }

    return $xml_data;
  }

  public function set_parameter($name, $value) {
    $this->parameters[$name] = $value;
  }

  public function set_parameters($params = array()) {
    foreach ($params as $n => $v) {
      $this->set_parameter($n, $v);
    }
  }

  public function get_parameter($name) {
    return $this->parameters[$name];
  }

  public function get_parameters($add_default = TRUE) {
    if ($add_default) {
      $this->add_default_parameters();//@TODO: we prolly don't want to add these to the ->parameters var, but rather only add them to this very output.
    }
    return $this->parameters;
  }

  public function get_default_parameters() {
    return array(
      'service' => 'query', //@TODO: replace with settable var.
      'outputType' => 'XML', //@TODO: should we make this settable? and in case of cns, not parse as xml, obviously
    );
  }

  public function add_default_parameters() {
    foreach ($this->get_default_parameters() as $n => $v) {
      $this->set_parameter($n, $v);
    }
  }

  /**
   * Parses the http_url and rebuilds it into scheme://host/path with an option al port
   */
  public function get_normalized_http_url() {
    $parts = parse_url(TixwayData::http_url());

    $port = (isset($parts['port']) && $parts['port'] != '80') ? ':' . $parts['port'] : '';
    $path = (isset($parts['path'])) ? $parts['path'] : '';

    return $parts['scheme'] . '://' . $parts['host'] . $port . $path;
  }

  /**
   * builds the data one would send in a POST request
   */
  public function to_postdata() {
    $total = array();
    foreach ($this->get_parameters() as $k => $v) {
      if (is_array($v)) {
        foreach ($v as $partial) {
          $v_concat[] = TixwayUtil::urlencodeRFC3986($partial);
        }
        $v = implode('+', $v_concat);
      }
      $total[] = TixwayUtil::urlencodeRFC3986($k) . "=" . TixwayUtil::urlencodeRFC3986($v);
    }
    $out = implode("&", $total);
    return $out;
  }
}

class TixwayQuery extends Tixway {
  private $parameters;

  function __construct($user, $password, $parameters = array()) {
    //I do not grok the way PHP extends classes yet not call its parent constructor. So lets include it here.
    parent::__construct($user, $password, $parameters);
  }

  public function get_single_movie() {

  }

  public function find_cinemas($filter = array()) {
    $cinemas = array();

    $data = TixwayData::result_fields_classes('cinema');
    $this->set_parameter('output', $data['cinema']);

    $this->set_parameter('cinemaID', implode(',', range(1,999)));//@TODO: make $start and $end GLOBALS in case the API changes the range.
    $cinemas = $this->do_call();
    return $cinemas;
  }

  /**
  * Finds cities that fit within the filter
  * @param filter array A keyed array where the key resembles the field and the value must match a record in the results.
  * E.g. find_cities(array('cityName' => 'Nijmegen')) will return only the cities where the name equals Nijmegen.
  * @todo: add support for non-city params, so that we can search for e.g. cities with a premiere. Or cities where a film is performed with a price under €2,- etceteras.
  * @todo: allow reg-exps in values.
  * @todo: allow options for OR and AND,somehow.
  */
  public function find_cities($filter = array()) {
    $cities = array();

    $data = TixwayData::result_fields_classes('city');
    $this->set_parameter('output', $data['city']);

    //Somehow this does not work, I get only one city, being "My City".
    $this->set_parameter('cityID', implode(',', range(1,999)));//@TODO: make $start and $end GLOBALS in case the API changes the range.
    $cities = $this->do_call();
    return $cities;
  }

  /**
  * Finds movies that fit within the filter
  * @param filter array A keyed array where the key resembles the field and the value must match a record in the results.
  * E.g. find_movies(array('movieType' => '1')) will return only the premiere movies.
  * @todo: add support for non-city params, so that we can search for e.g. movies in city X. Or movies with a performance with price under €2,- etceteras.
  * @todo: allow reg-exps in values.
  * @todo: allow options for OR and AND,somehow.
  * WARNING, this gives LARGE results, potentially containing 10 000 movies. Use with care!
  *          even when filtered, we must still collect all 10 0000 movies. 
  */
  public function find_movies($filter = array()) {
    $data = TixwayData::result_fields_classes('movie');
    $this->set_parameter('output', $data['movie']);
    $movies = array();

    //10 000 items is far too much to call at once. Lets do this in concurrent calls of hundred each.
    $steps = 100;$top = 9999;//@TODO: make $start and $end defines in case the API changes the range
    for ($i = 3500; $i <= $top; ($i = $i + $steps)) {
      $result = array();
      //range should start at $i and end at the next 1000, but never get higher then 9999.
      $next = ($i + $steps) -1;
      $range = range($i, min($next, $top));
      $this->set_parameter('movieID', implode(',', $range));
      $result = $this->do_call(); //TODO: find out if we can look for empty results, and in case of empty results bread out of this loop. i.e. is it safe to assume that all IDs upto X are filled when X is the highest ID?
      $movies = array_merge($movies, $result);
    }

    return $movies;
  }


  public function get_single_cinema($cinema_id) {
    //'cinemaID', 'cinemaName', 'cinemaPhoneNo', 'cinemaOV', 'cityName', 'cityID',//@TODO: preg_replace <br> with <br/> in cinemaOV
  }

  public function get_all_movies_in_cities($city_ids = array()) {

  }

  public function get_all_movies_in_cinemas($cinema_ids = array()) {
    $items = array();

    $this->set_parameters = $params;
    $output_fields  = array(
      'cinemaID',
      'movieName',
      'movieNameNormal',
      'movieDescription',//@TODO: preg_replace <br> with <br/>
      'movieURL', // (the url of the movie);
      'movieAge', /* (the ranking of the movie). This is a combination of the following  keywords, white space separated:
         - al (all ages);
         - al+ (parental guidance);
         - 12;
         - 16;
         - geweld (violence);
         - seks (sex);
         - angst (fear);
        - drugs;
        - discr (discrimination);
        - grof (rude).*/
      'movieType', // (the type of the movie, 0 means normal, 1 - ? means “premiere”);
      'movieCount', // (the booking top count, 0 means no top place);
    );
    $this->set_parameter('output', $output_fields);
    $items = $this->do_call();
    return $items;
  }

  public function get_all_performances_in_cinemas($cinema_ids = array()) {
    $items = array();

    $this->set_parameters = $params;

    $output_fields  = array(
      'cinemaID',
      'movieID',
      'performanceTime', // (the starting time of the performance in hh:mm fomat, 24 hour  notation);
      'performanceDate', //  (the starting date of the performance in dd-mm-yyyy format);
      'performanceType', /*  (the type of performance). One of:
        - I, (information) - active for information;
        - R, (reservation) - active for reservation;
        - L, (limited sale) - active for sale only (not for reservation);
        - S, (sale) - active for reservation and sale.*/
      'performancePrice', //  (the price of the performance). Defaults to Euro if no channel is specified;
      'performancePriceInfo', /*  (the priceinformation of the performance). The channel for which prices are reported
        defaults to Euro if no channel is specified. Consists of  the following information per price:
        - priceID (the BelBios price ID);
        - priceName (the name of the price);
        - price (the price in smallest units for the active channel);
        - maxReservation (maximum number of tickets allowed to reserve in one tion);
        - maxSale (maximum number of tickets allowed to buy in one transaction).*/
      'channel', /*  (the sale channel, that is a detailing of the information for a sale  performance). One of:
        - E, (Euro) – active for Euro sale;
        - A, (AirMiles) – active for AirMiles sale;
        - Pxxxxxx, (Prepaid) – active for Prepaid card type xxxxxx sale. Prepaid card types range from 011000 – 999999.*/
      'performanceScreenID', //. The venues screen number of the space where the performance takes place
      'performanceScreenName', //. The venues screen name of the space where the performance takes place
    );

    $this->set_parameter('output', $output_fields);
    $cinema_ids = array_map(array('TixwayUtil', 'urlencodeRFC3986'), $cinema_ids);//normalise arguments.
    $this->set_parameter('cinemaID', implode(',', $cinema_ids));

    $items = $this->do_call();
    return $items;
  }

  public function get_all_performances_for_movies($movie_ids = array()) {
    $items = array();

    $this->set_parameters = $params;
    $output_fields  = array(
      'performanceTime',
      'performanceDate',
      'performanceScreenID',
      'performanceScreenName',
    );
    $this->set_parameter('output', $output_fields);

    $movie_ids = array_map(array('TixwayUtil', 'urlencodeRFC3986'), $movie_ids);//normalise arguments.
    $this->set_parameter('movieID', implode(',', $movie_ids));

    $items = $this->do_call();
    return $items;
  }
}

class TixwayTransaction extends Tixway {

}

class TixwayUtil {
  public static function urlencodeRFC3986($string) {
    return str_replace('%7E', '~', rawurlencode($string));
  }

  public static function urldecodeRFC3986($string) {
    return rawurldecode($string);
  }
}

class TixwayData {

  public function http_url() {
    return 'https://ras.belbios.nl/query.php';
  }

  /**
   * Returns fields that affect the query results. In addition to required fields such as type, username etceteras
   */
  public static function query_fields() {
    return array(
      'cityID', //The cityID to select for, range 0 – 999, comma-seperated list allowed.
      'cinemaID', //The cinemaID to select for, range 0 – 999, comma-seperated list allowed.
      'groupID', //The groupID to select for, range 0 – 99, comma-seperated list allowed.
      'movieID', //The movieID to select for, range 0 – 9999, comma-seperated list allowed.
      'type', //The type of performance to select for, range I, R, L, S. Comma-seperated list allowed.
      'rank', //The rank of a performance to select for. All ranks with a name which contains the  specified string will match. Matching is case insensitive. Only performances with a matching rank name, active for the specified performance type will be reported.
    );
  }

  /**
   * Returns fields that can be included in the resultset.
   * Filtering on one of these performs bad, since the complete set will be returned by tixway and then stripped locally. Potentially returning and sifting trhough tenthousand results.
   */
  public static function result_fields() {
    return array(
      'cityID', // (the BelBios city ID);
      'cityName', // (the BelBios city name);
      'movieID', // (the BelBios movie ID);
      'distributorMovieID', //(the movie ID as issued by movie distributors themselves);
      'movieName', // (the BelBios movie name, adapted for drop-down box selection);
      'movieNameNormal', // (the normal BelBios movie name);
      'movieDescription', // (the BelBios movie description); Newlines in the movie description text are presented as <br>
      'movieURL', // (the url of the movie);
      'movieAge', /* (the ranking of the movie). This is a combination of the following
           keywords, white space separated:
           al (all ages);
           al+ (parental guidance);
           12;
           16;
           geweld (violence);
           seks (sex);
           angst (fear);
           drugs;
           discr (discrimination);
           grof (rude).*/
      'movieType', // (the type of the movie, 0 means normal, 1 - ? means “premiere”);
      'movieCount', // (the booking top count, 0 means no top place);
      'cinemaID', // (the BelBios cinema ID);
      'cinemaName', // (the BelBios cinema name);
      'cinemaOV', // (the BelBios cinema public transportation information);  Newlines in the cinema OV description text are presented as <br>
      'cinemaPhoneNo', // (the phoneNo on which the cinema is reachable);
      'performanceTime', // (the starting time of the performance in hh:mm fomat, 24 hour notation);
      'performanceDate', // (the starting date of the performance in dd-mm-yyyy format);
      'performanceType', /* (the type of performance). One of:
           I, (information) - active for information;
           R, (reservation) - active for reservation;
           L, (limited sale) - active for sale only (not for reservation);
           S, (sale) - active for reservation and sale.*/
      'performancePrice', // (the price of the performance). Defaults to Euro if no channel is  specified;
      'performancePriceInfo', /* (the priceinformation of the performance). The channel for
           which prices are reported defaults to Euro if no channel is specified. Consists of
           the following information per price:
           priceID (the BelBios price ID);
           priceName (the name of the price);
           price (the price in smallest units for the active channel);
           maxReservation (maximum number of tickets allowed to reserve in one
               transaction);
           maxSale (maximum number of tickets allowed to buy in one transaction). Output field performancePriceInfo is only supported for output type xml. Multiple
           prices for one performance are presented as multiple xml nodes. The sub fields
           are presented as xml leafs.*/
      'channel', /* (the sale channel, that is a detailing of the information for a sale
           performance). One of:
           E, (Euro) – active for Euro sale;
           A, (AirMiles) – active for AirMiles sale;
           Pxxxxxx, (Prepaid) – active for Prepaid card type xxxxxx sale. Prepaid card
               types range from 011000 – 999999.*/
      'tip', // (the current BelBios movie tip). When specifying tip, it overrides all other output fields, reporting only the tip.
      'performanceScreenID', //. The venues screen number of the space where the performance takes place
      'performanceScreenName', //. Then venues screen name of the space where the  performance takes place
    );
  }

  /**
   * Returns fields that can be included in the resultset, grouped by purpose. +
   * These resemble the fields that will be returned when querying for one of these items. E.g. all fields under 'movie' will be returned when requesting any list of movies. 
   * Filtering on one of these performs bad, since the complete set will be returned by tixway and then stripped locally. Potentially returning and sifting trhough tenthousand results.
   * @param $class string only return values for class with name string;
   */
  public static function result_fields_classes($class = '') {
    $classed = array(
      'movie' => array(
        'cityID',
        'cinemaID',
        'movieID',
        'distributorMovieID',
        'movieName',
        'movieNameNormal',
        'movieDescription',
        'movieURL',
        'movieAge',
        'movieType',
        'movieCount',
      ),
      'performance' => array(
        'cityID',
        'cinemaID',
        'movieID',
        'performanceTime',
        'performanceDate',
        'performanceType',
        'performancePrice',
        'performancePriceInfo',
        'channel',
        'performanceScreenID',
        'performanceScreenName',
      ),
      'city' => array(
        'cityID',
        'cityName',
      ),
      'cinema' => array(
        'cinemaID',
        'cinemaName',
        'cinemaOV',
        'cinemaPhoneNo',
      ),
      'meta' => array(//Unrelated values. 
        'tip',
      ),
    );

    if (!empty($class) && in_array($class, $classed)) {
      $out[$class] = $classed[$class];//sicking in arrays, to make sure we always return same structure
    }
    else {
      $out = $classed;
    }

    return $out;
  }

  public static function ras_error($code) {
    $error = '';
    $code = (int) $code;
    $errors = array(
      1 => 'wrong service',
      2 => 'wrong delivery',
      3 => 'wrong cinemaID',
      4 => 'wrong movieID',
      5 => 'wrong performanceDate',
      6 => 'wrong performanceTime',
      7 => 'wrong channel',
      08 => 'wrong customerID',
      09 => 'wrong numberOfTickets',
      10 => 'wrong priceID',
      11 => 'wrong email address',
      12 => 'wrong info',
      13 => 'wrong transactionID',
      20 => 'fatal error',
      21 => 'connect, cinema cannot be reached',
      22 => 'reservation, reservation cannot be made locally',
      23 => 'online, out of buffers centrally and not going online',
      24 => 'maxuser, maximum number of reservations per customer reached',
      25 => 'maxperf, maximum number of reservations per performance reached',
      26 => 'exist, customer ID/transactionID combination does not exist or is expired',
      27 => 'pending, a payment for this customer is pending, no new payment is accepted',
    );
    if  ( in_array($code, $errors) ) {
      $error = $errors[$code];
    }

    return $error;
  }
}

//Closing tag intentionally left out. See http://drupal.org/node/545 for the reasoning