### 0. Соревнование

Адрес контеста: https://contest.yandex.ru/contest/5513/enter/

### 1. Свайп и мобильные клавиатуры

Свайп (swipe) - один из методов ввода текста в мобильных клавиатурах, впервые появившийся в клавиатуре Swype от Nuance. Утверждается, что при помощи этого метода ввода можно достигать невероятной скорости ввода при использовании мобильных телефонов!
Безотносительно истинности этого утверждения, этот способ ввода чрезвычайно популярен у некоторых групп пользователей. Так, летом 2017 года примерно 20% отзывов пользователей Я.Клавиатуры для Android содержали просьбу внедрить свайп :)

Самое сложное при реализации свайпа - написать метод, который будет определять, какое именно слово вводил пользователь. В этом задании вам предстоит реализовать именно такой метод!

Вам будет предоставлена коллекция данных, содержащая несколько тысяч примеров введённых пользователями кривых и соответствующих этим кривым слов. Необходимо будет построить метод, который угадывает как можно больше слов. Данные разбиты на три множества: обучающее, валидационное и тестовое в соотношениях, соответственно, 40%, 20%, 40%.

Данные для обучения: https://github.com/ashagraev/swipe/blob/master/swipe.train
Данные для теста: https://github.com/ashagraev/swipe/blob/master/swipe.test

Часть тестовых данных используется для получения публичной оценки качества, часть будет использована после завершения контеста для получения итоговых результатов.

### 2. Формат данных

По одной строчке на задание. Файлы - tsv из трёх колонок.
Первая колонка: описание раскладки клавиатуры. Содержит ряд записей для каждой буквы в раскладке, записи разделены пробелами. Каждая запись содержит четыре числа и одну букву; эти поля разделены двоеточием. Рассмотрим, например, следующую запись:

```
2.500000:3.594249:22.777778:30.431310:й
```

Она означает, что буква  й имеет координаты левого верхнего угла по горизонтали  2.5 , по вертикали примерно  3.6 , при этом ширина кнопки равна примерно  22.8 , а высота -  30.4 . Координаты верхнего левого угла клавиатуры -  0, 0 .
Вторая колонка: описание кривой, введённой пользователем. Ряд записей, разделённых пробелами:

```
98:103 99:102 99:99 99:96 106:86 112:80 118:73 121:69
```

Третья колонка - введённое слово. Присутствует только в данных для обучения.

Кроме того, в соревновании имеется небольшой словарь русского языка, который гарантированно содержит все слова, встречающиеся в соревновании.
https://github.com/ashagraev/swipe/blob/master/ru.words.small

### 3. Проверка решений

Скрипт для проверки решений устроен очень просто. Пример того, как мог бы выглядеть скрипт: https://github.com/ashagraev/swipe/blob/master/checker.py
Фактически он чекера в контесте иной, что связано с некоторыми особенностями проверяющей системы, но содержательных отличий нет.

Пример запуска скрипта на модельных данных из контеста:
```shell
python ./checker.py --target sample_targets.txt --submission sample_submission.txt
0.7
```

### 4. Бейзлайн

Пример очень плохого бейзлайна с качеством примерно 10%:
https://github.com/ashagraev/swipe/tree/master/baseline

Это очень, очень испорченное первое решение, которое я когда-либо делал для свайпа, аккуратно переписанное на stl.
Содержит несколько возможностей для улучшения, и, что важнее, примеры кода для парсинга данных, работы с utf8-строками и т.п.

Запускать программу нужно следующим образом:
```shell
baseline --dict ru.words.small --tasks swipe.train
```

На stderr будет выведено некоторое количество отладочной информации, в т.ч. вычисленную оценку точности на файле, если файл содержит правильные ответы.
На stdout - файл решения. При запуске на файле swipe.test получится файл, пригодный для контеста, но оценка точности, конечно же, будет нулевой.

Пример запуска:
```shell
making clusters...
score: 40.1797
score: 35.4402
score: 34.7629
score: 34.4512
score: 34.2657
building vp tree...
built all!
10 processed...
20 processed...
30 processed...
40 processed...
50 processed...
...
2320 processed...
2330 processed...
2340 processed...
accuracy: 0.109355
```

### 5. Качество решений

Решения, которые получат более 90% правильных ответов, однозначно смогут помочь нашему продакшену.
Решения со скором более 80% наверняка будут содержать какие-то полезные для нас идеи.
Решения со скором более 60% можно считать нормальными.