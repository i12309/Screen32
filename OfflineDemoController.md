Нужно сделать отдельный класс OfflineDemoController для Screen32,
чтобы offline_demo=1 работал не через случайный пример в shared_app,
а через отдельный управляемый слой поведения.

Цель:
дать удобный способ задавать:
- порядок страниц
- какие кнопки куда ведут
- next/prev/goto rules
- простую локальную demo-навигацию без связи

Что нужно сделать:

1. Добавить новый слой в Screen32/common_app:
   - offline_demo_controller.h
   - offline_demo_controller.cpp

2. Роль OfflineDemoController:
   - работать только в режиме offline_demo=1
   - использовать уже существующий UI через page_id/element_id
   - не зависеть от transport/proto
   - не быть частью EezLvglAdapter

3. Минимальный API:
   - init(...)
   - setPageOrder(...)
   - bindButtonToNext(element_id)
   - bindButtonToPrev(element_id)
   - bindButtonToGoto(element_id, target_page_id)
   - onButtonEvent(...)
   - start(start_page_id)
   - currentPage()

4. Источник ids:
   - использовать generated page_ids / element_ids из meta generator
   - не дублировать ids вручную по нескольким местам

5. Поведение:
   - allow cycling pages in configured order
   - next/prev/goto по кнопкам
   - start page берется из frontend config
   - если правила не заданы, контроллер ничего не ломает

6. Интеграция с frontend runtime:
   - при offline_demo=1 frontend runtime не требует backend transport
   - входящие UI button events идут в OfflineDemoController
   - OfflineDemoController вызывает showPage локально
   - transport/proto в этом режиме не обязателен

7. Убрать текущий ручной пример навигации из shared_app после миграции.
   - direct routes / back routes из demo-кода убрать, когда новый контроллер заработает
   - shared_app должен остаться тонким bootstrap-слоем

8. README:
   - описать offline_demo mode
   - показать, где задается:
     * page order
     * start page
     * button bindings

Критерий готовности:
- offline_demo=1 работает через отдельный OfflineDemoController
- порядок страниц задается явно
- кнопки next/prev/goto задаются явно
- текущий ручной пример навигации можно удалить