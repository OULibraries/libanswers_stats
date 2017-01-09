<?php

/**
 * @file
 *   Views query plugin for libanswers stats.
 */
class libanswers_stats_plugin_query extends views_plugin_query {
//  use GuzzleHttp\Client;

  function query($get_count = FALSE) {
  }

  /**
   * @param view $view
   * gets number live counts from labstats
   */
  function execute(&$view) {
    //get the anid that corresponds to the dataset choice
    $typeOfInteraction = $this->options['libanswers_dataset'];
    switch ($typeOfInteraction) {
      case 'chat':
        $anid = '1753';
        break;
      case 'email':
        $anid = '2474';
        break;
      case 'sms':
        $anid = '2476';
        break;
      case 'person':
        $anid = '2628';
        break;
      case 'telephone':
        $anid = '2629';
        break;
      default:
        $anid = '1753';
    }

    if (!isset($row)) {
      if ($cache = cache_get('libanswers' . $anid)) {
        $row = $cache->data;
      }
      else {
        //number of days, months, etc
        $incNum = $this->options['incnum'];
        //option for days, months, etc
        $spanOption = $this->options['spanOption'];
        //@todo add better error handling here
        //password and username variables from the drupal configuration
        $libanswers_stats_un = variable_get('libanswers_stats_un', '');
        $libanswers_stats_pw = variable_get('libanswers_stats_pw', '');
        if ($libanswers_stats_un == '' || $libanswers_stats_pw == '') {
          return;
        }
        //getting hours if hours is the option selected
        $startHours = ($spanOption == 'hours') ? $this->getHours($incNum, $spanOption) : '';
        $startDate = $this->getStartTime($incNum, $spanOption);
        $client = new GuzzleHttp\Client([
          'defaults' => [
            'allow_redirects' => [
              'max' => 20,        // allow at most 10 redirects.
              'strict' => TRUE,      // use "strict" RFC compliant redirects.
              'track_redirects' => TRUE,
            ]
          ]
        ]);
        $jar = new GuzzleHttp\Cookie\CookieJar();

        try {
        $response = $client->get('https://ou.libapps.com/libapps/login.php', ['cookies' => $jar]);
        $jar_array = $jar->toArray();
        $idtn = in_array('idtn', $jar_array[0]) ? $jar_array[0]['Value'] : $jar_array[1]['Value'];


        $response_login = $client->post('https://ou.libapps.com/libapps/login_process.php', [
          'cookies' => $jar,
          'headers' => [
            "Referer" => "https://ou.libapps.com/libapps/login.php"
          ],
          'body' => [
            'action' => '1',
            'site_id' => '0',
            'app_id' => '0',
            'target' => '',
            'public' => '0',
            'mode' => '0',
            's-libapps-email' => $libanswers_stats_un,
            's-libapps-password' => $libanswers_stats_pw,
            'idtn' => $idtn,
          ]
        ]);
        $data = $response_login->getStatusCode();
        if ($data != "200") {
          exit("bad response code, don't log in");
        }

        $client->get('https://ou.libapps.com/libapps/', ['cookies' => $jar]);

        # hit login page and get redirected through login process to accumulate desired cookies
        $response_login_step2 = $client->get('https://ou.libapps.com/libapps/login.php?site_id=6823'
          , ['cookies' => $jar, 'allow_redirects' => TRUE,]);

        $tempfile = tempnam(sys_get_temp_dir(), 'tempfile');

        $response_data = $client->get('http://libanswers.ou.edu/process_tally.php?m=exportxls2&iid=1355&anid=' . $anid . '&list=stats&start=' . $startDate . '&hrstart=' . $startHours, [
          'cookies' => $jar,
          'save_to' => $tempfile,
        ]);

        $temp = fopen($tempfile, 'r');
        //setting the file position to 0
        fseek($temp, 0);

        $columnHeads = TRUE;
        $tally = 0;
        while (($csvrow = fgetcsv($temp)) !== FALSE) {
          //we don't want the first row...which is the column titles
          if ($columnHeads) {
            $columnHeads = FALSE;
            continue;
          }
          $tally++;
        }
        } catch (\Exception $e) {
          watchdog('redirect', $e->getMessage());
        }
        $row = new stdClass();
        $row->count = isset($tally) ? $tally : 0;
        $row->title = $typeOfInteraction;

        cache_set('libanswers' . $anid, $row, 'cache', time() + 36000);
      }
    }
    $view->result[] = $row;
  }

  /**
   * @param int $inc
   * @param int $span
   * @return string
   *
   * set the default date to the proper format
   */
  function getStartTime($inc = 0, $span = 0) {
    //setting a start date if start date was deleted
    if ($span == 'hours') {
      $time = date('Y-m-d H:i:s');
    }
    else {
      $time = date('Y-m-d H:i:s', strtotime('-' . $inc . ' ' . $span . ' '));
    }
    $newDate = new DateTime($time, new DateTimeZone('UTC'));
    $newDate->setTimezone(new DateTimeZone('America/Chicago'));
    $tempStartDate = $newDate->format('Y-m-d');
    return $tempStartDate;
  }

  function getHours($inc = 0, $span = 0) {
    //setting a start hour if hour is the option passed in
    //set the entered time and midnight to the same format
    $time = date('Y-m-d H:i:s', strtotime('-' . $inc . ' ' . $span . ' '));
    $midnight = date('Y-m-d H:i:s', strtotime('today midnight'));

    //set the entered date to central timezone and keep midnight UTC
    $newDate = new DateTime($time, new DateTimeZone('UTC'));
    $newMidnight = new DateTime($midnight, new DateTimeZone('UTC'));
    $newDate->setTimezone(new DateTimeZone('America/Chicago'));

    //change both variables back to the same format for comparison
    $strTime = strtotime($newDate->format('Y-m-d H:i:s'));
    $strMidnight = strtotime($newMidnight->format('Y-m-d H:i:s'));

    //send back a starting hour of zero if the entered time is
    //previous to midnight
    if ($strTime < $strMidnight) {
      $tempStartHour = $newMidnight->format('H');
    }
    else {
      $tempStartHour = $newDate->format('H');
    }
    return $tempStartHour;
  }

  /**
   * @return array
   *
   * option defaults for query settings
   */
  function option_definition() {
    $options = parent::option_definition();

    $options['libanswers_dataset'] = array(
      'default' => 'chat',
    );
    $options['incnum'] = array(
      'default' => 1,
    );
    $options['spanOption'] = array(
      'default' => 'months',
    );

    return $options;
  }

  /**
   * @param $form
   * @param $form_state
   *
   * Creating the fields for the query settings view
   */
  function options_form(&$form, &$form_state) {
    $form = parent::options_form($form, $form_state);

    $form['libanswers_dataset'] = array(
      '#title' => t('Dataset Options'),
      '#type' => 'select',
      '#description' => 'Select the dataset for which you want results',
      '#options' => array(
        'chat' => 'Chat',
        'email' => 'Email a Librarian',
        'sms' => 'SMS',
        'person' => 'In Person',
        'telephone' => 'Telephone',
      ),
      '#default_value' => (isset($this->options['libanswers_dataset'])) ? $this->options['libanswers_dataset'] : 'chat',
    );
    $form['incnum'] = array(
      '#type' => 'textfield',
      '#title' => t('Incremental Number'),
      '#description' => t('Enter the incremental number to coincide with the span field below'),
      '#default_value' => $this->options['incnum'],
      '#maxlength' => 10,
      '#size' => 10,
    );
    $form['spanOption'] = array(
      '#type' => 'select',
      '#title' => t('Span of time'),
      '#description' => t('Select the option to go with the number entered in the number field above.'),
      '#options' => array(
        'hours' => t('hours ago'),
        'days' => t('days ago'),
        'months' => t('months ago'),
        'years' => t('years ago'),
      ),
      '#default_value' => (isset($this->options['spanOption'])) ? $this->options['spanOption'] : 'days',
    );
  }
}
